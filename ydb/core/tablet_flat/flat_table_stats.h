#pragma once
#include "defs.h"

namespace NKikimr {
namespace NTable {

    struct TPartView;

    struct TPartStats {
        ui64 PartsCount = 0;    /* Total used TPart units in db    */
        ui64 IndexBytes = 0;
        ui64 OtherBytes = 0;    /* Other metadata and sys. indexes */
        ui64 ByKeyBytes = 0;
        ui64 PlainBytes = 0;    /* Plain data pages size           */
        ui64 CodedBytes = 0;    /* Encoded data pages size         */
        ui64 SmallBytes = 0;    /* Size of outer blobs in page collections  */
        ui64 SmallItems = 0;    /* Outer blobs count in page collections    */
        ui64 LargeBytes = 0;    /* Size of external blobs in parts */
        ui64 LargeItems = 0;    /* External blobs count in parts   */
        ui64 RowsErase = 0;     /* Rows with ERowOp::Erase codes     */
        ui64 RowsTotal = 0;     /* Total count of rows             */

        explicit operator bool() const {
            return PartsCount > 0;
        }

        void Add(const TPartView& partView);
        bool Remove(const TPartView& partView);

        TPartStats& operator+=(const TPartStats& rhs);
        TPartStats& operator-=(const TPartStats& rhs);
    };

    struct TSelectStats {
        ui64 Sieved = 0;
        ui64 Weeded = 0;
        ui64 NoKey = 0;
        ui64 Invisible = 0;
    };

    struct TCompactionStats {
        THashSet<ui64> PartOwners;
        ui64 PartCount = 0;
        ui64 MemRowCount = 0;
        ui64 MemDataSize = 0;
        ui64 MemDataWaste = 0;
    };

}
}
