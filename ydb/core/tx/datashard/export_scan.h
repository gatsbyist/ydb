#pragma once

#include "defs.h"

#include <ydb/core/tablet_flat/flat_scan_iface.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NKikimr {
namespace NDataShard {

struct TEvExportScan {
    enum EEv {
        EvReset = EventSpaceBegin(TKikimrEvents::ES_PRIVATE),
        EvReady,
        EvFeed,
        EvBuffer,
        EvFinish,

        EvEnd,
    };

    static_assert(EvEnd < EventSpaceEnd(TKikimrEvents::ES_PRIVATE), "expect EvEnd < EventSpaceEnd(TKikimrEvents::ES_PRIVATE)");

    struct TEvReset: public TEventLocal<TEvReset, EvReset> {};
    struct TEvReady: public TEventLocal<TEvReady, EvReady> {};
    struct TEvFeed: public TEventLocal<TEvFeed, EvFeed> {};

    template <typename TBuffer>
    struct TEvBuffer: public TEventLocal<TEvBuffer<TBuffer>, EvBuffer> {
        TBuffer Buffer;
        bool Last;

        TEvBuffer() = default;

        explicit TEvBuffer(TBuffer buffer, bool last)
            : Buffer(std::move(buffer))
            , Last(last)
        {
        }

        TString ToString() const override {
            return TStringBuilder() << this->ToStringHeader() << " {"
                << " Last: " << Last
            << " }";
        }
    };

    struct TEvFinish: public TEventLocal<TEvFinish, EvFinish> {
        bool Success;
        TString Error;

        TEvFinish() = default;

        explicit TEvFinish(bool success, const TString& error = TString())
            : Success(success)
            , Error(error)
        {
        }

        TString ToString() const override {
            return TStringBuilder() << ToStringHeader() << " {"
                << " Success: " << Success
                << " Error: " << Error
            << " }";
        }
    };

}; // TEvExportScan

struct TExportScanProduct: public IDestructable {
    bool Success;
    TString Error;
    ui64 BytesRead;
    ui64 RowsRead;

    explicit TExportScanProduct(bool success, TString error, ui64 bytes, ui64 rows)
        : Success(success)
        , Error(std::move(error))
        , BytesRead(bytes)
        , RowsRead(rows)
    {
    }

}; // TExportScanProduct

namespace NExportScan {

class IBuffer {
public:
    using TPtr = THolder<IBuffer>;

public:
    virtual ~IBuffer() = default;

    virtual void ColumnsOrder(const TVector<ui32>& tags) = 0;
    virtual void Collect(const NTable::IScan::TRow& row) = 0;
    virtual IEventBase* PrepareEvent(bool last) = 0;
    virtual void Clear() = 0;
    virtual bool IsFilled() const = 0;
    virtual ui64 GetRows() const = 0;
    virtual ui64 GetBytesRead() const = 0;
    virtual ui64 GetBytesSent() const = 0;
};

} // NExportScan

NTable::IScan* CreateExportScan(NExportScan::IBuffer::TPtr buffer, std::function<NActors::IActor*()>&& createUploaderFn);

} // NDataShard
} // NKikimr
