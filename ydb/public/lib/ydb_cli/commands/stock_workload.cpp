#include "stock_workload.h"

#include <ydb/library/workload/stock_workload.h>
#include <ydb/library/workload/workload_factory.h>
#include <ydb/public/lib/ydb_cli/commands/ydb_common.h>

namespace NYdb::NConsoleClient {

NTable::TSession TWorkloadCommand::GetSession() {
    NTable::TCreateSessionResult result = TableClient->GetSession(NTable::TCreateSessionSettings()).GetValueSync();
    ThrowOnError(result);
    return result.GetSession();
}

TCommandStock::TCommandStock()
    : TClientCommandTree("stock", {}, "YDB stock workload")
{
    AddCommand(std::make_unique<TCommandStockInit>());
    AddCommand(std::make_unique<TCommandStockRun>());
}

TCommandStockInit::TCommandStockInit()
    : TWorkloadCommand("init", {}, "Create and initialize tables for workload")
    , ProductCount(0)
    , Quantity(0)
    , MinPartitions(0)
    , PartitionsByLoad(true)
{}

void TCommandStockInit::Config(TConfig& config) {
    TYdbCommand::Config(config);

    config.SetFreeArgsNum(0);

    config.Opts->AddLongOption('p', "products", "Product count. Value in 1..500 000.")
        .DefaultValue(100).StoreResult(&ProductCount);
    config.Opts->AddLongOption('q', "quantity", "Quantity of each product in stock.")
        .DefaultValue(1000).StoreResult(&Quantity);
    config.Opts->AddLongOption('o', "orders", "Initial orders count.")
        .DefaultValue(100).StoreResult(&OrderCount);
    config.Opts->AddLongOption("min-partitions", "Minimum partitions for tables.")
        .DefaultValue(40).StoreResult(&MinPartitions);
    config.Opts->AddLongOption("auto-partition", "Enable auto partitioning by load.")
        .DefaultValue(true).StoreResult(&PartitionsByLoad);
}

void TCommandStockInit::Parse(TConfig& config) {
    TClientCommand::Parse(config);
}

int TCommandStockInit::Run(TConfig& config) {
    if (ProductCount > 500'000) {
        throw TMissUseException() << "Product count must be in range 1..500 000." << Endl;
    }

    Driver = std::make_unique<NYdb::TDriver>(CreateDriver(config));
    TableClient = std::make_unique<NTable::TTableClient>(*Driver);
    NYdbWorkload::TStockWorkloadParams params;
    params.DbPath = config.Database;
    params.ProductCount = ProductCount;
    params.Quantity = Quantity;
    params.OrderCount = OrderCount;
    params.MinPartitions = MinPartitions;
    params.PartitionsByLoad = PartitionsByLoad;

    NYdbWorkload::TWorkloadFactory factory;
    auto workloadGen = factory.GetWorkloadQueryGenerator("stock", &params);
    if (workloadGen.get() == nullptr) {
        throw TMissUseException() << "Invalid path to database." << Endl;
    }

    auto session = GetSession();
    auto result = session.ExecuteSchemeQuery(workloadGen->GetDDLQueries()).GetValueSync();
    ThrowOnError(result);

    auto queryInfoList = workloadGen->GetInitialData();
    for (auto queryInfo : queryInfoList) {
        auto prepareResult = session.PrepareDataQuery(queryInfo.Query.c_str()).GetValueSync();
        if (!prepareResult.IsSuccess()) {
            Cerr << "Prepare failed: " << prepareResult.GetIssues().ToString() << Endl
                << "Query:\n" << queryInfo.Query << Endl;
            return EXIT_FAILURE;
        }

        auto dataQuery = prepareResult.GetQuery();
        auto result = dataQuery.Execute(NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
                                        std::move(queryInfo.Params)).GetValueSync();
        if (!result.IsSuccess()) {
            Cerr << "Query execution failed: " << result.GetIssues().ToString() << Endl
                << "Query:\n" << queryInfo.Query << Endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

TCommandStockRun::TCommandStockRun()
    : TClientCommandTree("run", {}, "Run YDB stock workload")
{
    AddCommand(std::make_unique<TCommandStockRunInsertRandomOrder>());
    AddCommand(std::make_unique<TCommandStockRunSubmitRandomOrder>());
    AddCommand(std::make_unique<TCommandStockRunSubmitSameOrder>());
    AddCommand(std::make_unique<TCommandStockRunGetRandomCustomerHistory>());
    AddCommand(std::make_unique<TCommandStockRunGetCustomerHistory>());
}

TCommandStockRunInsertRandomOrder::TCommandStockRunInsertRandomOrder()
    : TWorkloadCommand("insertRandomOrder", {}, "Inserts orders with random ID")
    , ProductCount(0)
{}

void TCommandStockRunInsertRandomOrder::Config(TConfig& config) {
    TWorkloadCommand::Config(config);

    config.SetFreeArgsNum(0);

    config.Opts->AddLongOption('p', "products", "Products count to use in workload.")
        .DefaultValue(100).StoreResult(&ProductCount);
}

void TCommandStockRunInsertRandomOrder::Parse(TConfig& config) {
    TClientCommand::Parse(config);
}

int TCommandStockRunInsertRandomOrder::Run(TConfig& config) {
    PrepareForRun(config);

    NYdbWorkload::TStockWorkloadParams params;
    params.DbPath = config.Database;
    params.ProductCount = ProductCount;

    NYdbWorkload::TWorkloadFactory factory;
    auto workloadGen = factory.GetWorkloadQueryGenerator("stock", &params);
    if (workloadGen.get() == nullptr) {
        throw TMissUseException() << "Invalid path to database." << Endl;
    }

    return RunWorkload(workloadGen, static_cast<int>(NYdbWorkload::TStockWorkloadGenerator::EType::InsertRandomOrder));
}

TCommandStockRunSubmitRandomOrder::TCommandStockRunSubmitRandomOrder()
    : TWorkloadCommand("submitRandomOrder", {}, "Submit random orders")
    , ProductCount(0)
{}

void TCommandStockRunSubmitRandomOrder::Config(TConfig& config) {
    TWorkloadCommand::Config(config);

    config.SetFreeArgsNum(0);

    config.Opts->AddLongOption('p', "products", "Products count to use in workload.")
        .DefaultValue(100).StoreResult(&ProductCount);
}

void TCommandStockRunSubmitRandomOrder::Parse(TConfig& config) {
    TClientCommand::Parse(config);
}

int TCommandStockRunSubmitRandomOrder::Run(TConfig& config) {
    PrepareForRun(config);

    NYdbWorkload::TStockWorkloadParams params;
    params.DbPath = config.Database;
    params.ProductCount = ProductCount;

    NYdbWorkload::TWorkloadFactory factory;
    auto workloadGen = factory.GetWorkloadQueryGenerator("stock", &params);
    if (workloadGen.get() == nullptr) {
        throw TMissUseException() << "Invalid path to database." << Endl;
    }

    return RunWorkload(workloadGen, static_cast<int>(NYdbWorkload::TStockWorkloadGenerator::EType::SubmitRandomOrder));
}

TCommandStockRunSubmitSameOrder::TCommandStockRunSubmitSameOrder()
    : TWorkloadCommand("submitSameOrder", {}, "Submit orders with same products")
    , ProductCount(0)
{}

void TCommandStockRunSubmitSameOrder::Config(TConfig& config) {
    TWorkloadCommand::Config(config);

    config.SetFreeArgsNum(0);

    config.Opts->AddLongOption('p', "products", "Products count to use in workload.")
        .DefaultValue(100).StoreResult(&ProductCount);
}

void TCommandStockRunSubmitSameOrder::Parse(TConfig& config) {
    TClientCommand::Parse(config);
}

int TCommandStockRunSubmitSameOrder::Run(TConfig& config) {
    PrepareForRun(config);

    NYdbWorkload::TStockWorkloadParams params;
    params.DbPath = config.Database;
    params.ProductCount = ProductCount;

    NYdbWorkload::TWorkloadFactory factory;
    auto workloadGen = factory.GetWorkloadQueryGenerator("stock", &params);
    if (workloadGen.get() == nullptr) {
        throw TMissUseException() << "Invalid path to database." << Endl;
    }

    return RunWorkload(workloadGen, static_cast<int>(NYdbWorkload::TStockWorkloadGenerator::EType::SubmitSameOrder));
}

TCommandStockRunGetRandomCustomerHistory::TCommandStockRunGetRandomCustomerHistory()
    : TWorkloadCommand("getRandomCustomerHistory", {}, "Selects orders of random customer")
    , Limit(0)
{}

void TCommandStockRunGetRandomCustomerHistory::Config(TConfig& config) {
    TWorkloadCommand::Config(config);

    config.SetFreeArgsNum(0);

    config.Opts->AddLongOption('l', "limit", "Number of last orders to select.")
        .DefaultValue(10).StoreResult(&Limit);
}

void TCommandStockRunGetRandomCustomerHistory::Parse(TConfig& config) {
    TClientCommand::Parse(config);
}

int TCommandStockRunGetRandomCustomerHistory::Run(TConfig& config) {
    PrepareForRun(config);

    NYdbWorkload::TStockWorkloadParams params;
    params.DbPath = config.Database;
    params.Limit = Limit;

    NYdbWorkload::TWorkloadFactory factory;
    auto workloadGen = factory.GetWorkloadQueryGenerator("stock", &params);
    if (workloadGen.get() == nullptr) {
        throw TMissUseException() << "Invalid path to database." << Endl;
    }

    return RunWorkload(workloadGen, static_cast<int>(NYdbWorkload::TStockWorkloadGenerator::EType::GetRandomCustomerHistory));
}

TCommandStockRunGetCustomerHistory::TCommandStockRunGetCustomerHistory()
    : TWorkloadCommand("getCustomerHistory", {}, "Selects orders of 10000th customer")
    , Limit(0)
{}

void TCommandStockRunGetCustomerHistory::Config(TConfig& config) {
    TWorkloadCommand::Config(config);

    config.SetFreeArgsNum(0);

    config.Opts->AddLongOption('l', "limit", "Number of last orders to select.")
        .DefaultValue(10).StoreResult(&Limit);
}

void TCommandStockRunGetCustomerHistory::Parse(TConfig& config) {
    TClientCommand::Parse(config);
}

int TCommandStockRunGetCustomerHistory::Run(TConfig& config) {
    PrepareForRun(config);

    NYdbWorkload::TStockWorkloadParams params;
    params.DbPath = config.Database;
    params.Limit = Limit;

    NYdbWorkload::TWorkloadFactory factory;
    auto workloadGen = factory.GetWorkloadQueryGenerator("stock", &params);
    if (workloadGen.get() == nullptr) {
        throw TMissUseException() << "Invalid path to database." << Endl;
    }

    return RunWorkload(workloadGen, static_cast<int>(NYdbWorkload::TStockWorkloadGenerator::EType::GetCustomerHistory));
}

} // namespace NYdb::NConsoleClient {
