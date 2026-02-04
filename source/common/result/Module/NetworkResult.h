/// @file NetworkResult.h
/// @brief ネットワークエラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// ネットワークエラー
    namespace NetworkResult
    {

        //=========================================================================
        // 接続関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultConnectionError, result::ModuleId::Network, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultConnectionFailed, result::ModuleId::Network, 1);
        NS_DEFINE_ERROR_RESULT(ResultConnectionRefused, result::ModuleId::Network, 2);
        NS_DEFINE_ERROR_RESULT(ResultConnectionReset, result::ModuleId::Network, 3);
        NS_DEFINE_ERROR_RESULT(ResultConnectionAborted, result::ModuleId::Network, 4);
        NS_DEFINE_ERROR_RESULT(ResultConnectionTimeout, result::ModuleId::Network, 5);
        NS_DEFINE_ERROR_RESULT(ResultNotConnected, result::ModuleId::Network, 6);
        NS_DEFINE_ERROR_RESULT(ResultAlreadyConnected, result::ModuleId::Network, 7);
        NS_DEFINE_ERROR_RESULT(ResultConnectionClosed, result::ModuleId::Network, 8);
        NS_DEFINE_ERROR_RESULT(ResultNoRoute, result::ModuleId::Network, 9);
        NS_DEFINE_ERROR_RESULT(ResultHostUnreachable, result::ModuleId::Network, 10);
        NS_DEFINE_ERROR_RESULT(ResultNetworkUnreachable, result::ModuleId::Network, 11);

        //=========================================================================
        // ソケット関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultSocketError, result::ModuleId::Network, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultSocketCreateFailed, result::ModuleId::Network, 100);
        NS_DEFINE_ERROR_RESULT(ResultSocketBindFailed, result::ModuleId::Network, 101);
        NS_DEFINE_ERROR_RESULT(ResultSocketListenFailed, result::ModuleId::Network, 102);
        NS_DEFINE_ERROR_RESULT(ResultSocketAcceptFailed, result::ModuleId::Network, 103);
        NS_DEFINE_ERROR_RESULT(ResultSocketCloseFailed, result::ModuleId::Network, 104);
        NS_DEFINE_ERROR_RESULT(ResultSocketOptionFailed, result::ModuleId::Network, 105);
        NS_DEFINE_ERROR_RESULT(ResultAddressInUse, result::ModuleId::Network, 106);
        NS_DEFINE_ERROR_RESULT(ResultAddressNotAvailable, result::ModuleId::Network, 107);
        NS_DEFINE_ERROR_RESULT(ResultInvalidAddress, result::ModuleId::Network, 108);

        //=========================================================================
        // 送受信関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultTransferError, result::ModuleId::Network, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultSendFailed, result::ModuleId::Network, 200);
        NS_DEFINE_ERROR_RESULT(ResultReceiveFailed, result::ModuleId::Network, 201);
        NS_DEFINE_ERROR_RESULT(ResultMessageTooLarge, result::ModuleId::Network, 202);
        NS_DEFINE_ERROR_RESULT(ResultBufferFull, result::ModuleId::Network, 203);
        NS_DEFINE_ERROR_RESULT(ResultWouldBlock, result::ModuleId::Network, 204);
        NS_DEFINE_ERROR_RESULT(ResultPartialSend, result::ModuleId::Network, 205);
        NS_DEFINE_ERROR_RESULT(ResultPartialReceive, result::ModuleId::Network, 206);

        //=========================================================================
        // DNS関連 (300-399)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultDnsError, result::ModuleId::Network, 300, 400);
        NS_DEFINE_ERROR_RESULT(ResultDnsResolutionFailed, result::ModuleId::Network, 300);
        NS_DEFINE_ERROR_RESULT(ResultHostNotFound, result::ModuleId::Network, 301);
        NS_DEFINE_ERROR_RESULT(ResultServiceNotFound, result::ModuleId::Network, 302);
        NS_DEFINE_ERROR_RESULT(ResultDnsTimeout, result::ModuleId::Network, 303);

        //=========================================================================
        // プロトコル関連 (400-499)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultProtocolError, result::ModuleId::Network, 400, 500);
        NS_DEFINE_ERROR_RESULT(ResultProtocolNotSupported, result::ModuleId::Network, 400);
        NS_DEFINE_ERROR_RESULT(ResultInvalidProtocolData, result::ModuleId::Network, 401);
        NS_DEFINE_ERROR_RESULT(ResultHandshakeFailed, result::ModuleId::Network, 402);
        NS_DEFINE_ERROR_RESULT(ResultVersionMismatch, result::ModuleId::Network, 403);
        NS_DEFINE_ERROR_RESULT(ResultTlsError, result::ModuleId::Network, 404);
        NS_DEFINE_ERROR_RESULT(ResultCertificateError, result::ModuleId::Network, 405);

    } // namespace NetworkResult

} // namespace NS
