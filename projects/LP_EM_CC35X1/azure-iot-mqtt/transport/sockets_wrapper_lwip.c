/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file sockets_wrapper_lwip.c
 * @brief LWIP socket wrapper.
 */

#include "sockets_wrapper.h"
#include "uart_term.h"

/* Standard includes. */
#include <stdbool.h>
#include <string.h>

/* Lwip includes. */
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/ip.h"

/* DNS IF includes */
#include "dns_if.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
/*-----------------------------------------------------------*/

/*
 * DNS timeouts.
 */
#ifndef lwipdnsresolverMAX_WAIT_SECONDS
    #define lwipdnsresolverMAX_WAIT_SECONDS    ( 20 )
#endif

/*
 * convert from system ticks to seconds.
 */
#define TICK_TO_S( _t_ )     ( ( _t_ ) / configTICK_RATE_HZ )

/*
 * convert from system ticks to micro seconds.
 */
#define TICK_TO_US( _t_ )    ( ( _t_ ) * 1000 / configTICK_RATE_HZ * 1000 )
/*-----------------------------------------------------------*/

/*
 * Task handle to use for DNS resolution.
 */
static TaskHandle_t xDNSResolutionTaskHandle = NULL;

/*-----------------------------------------------------------*/

/*
 * Lwip DNS Found callback, compatible with type "dns_found_callback"
 * declared in lwip/dns.h.
 *
 * NOTE: this resolves only ipv4 addresses; calls to dns_gethostbyname_addrtype()
 * must specify dns_addrtype == LWIP_DNS_ADDRTYPE_IPV4.
 */
static void lwip_dns_found_callback( const char * ucName,
                                     const ip_addr_t * xIPAddr,
                                     void * pvCallbackArg )
{
    if( xDNSResolutionTaskHandle != NULL )
    {
        uint32_t * ulAddr = ( uint32_t * ) pvCallbackArg;

        if( xIPAddr != NULL )
        {
            *ulAddr = *( ( uint32_t * ) xIPAddr ); /* NOTE: IPv4 addresses only */
        }
        else
        {
            *ulAddr = 0;
        }

        xTaskNotifyGive( xDNSResolutionTaskHandle );
    }
}
/*-----------------------------------------------------------*/

uint32_t prvGetHostByName( const char * pcHostName )
{
    uint32_t ulAddr = 0;
    err_t xLwipError = ERR_OK;
    ip_addr_t xLwipIpv4Address;
    uint32_t ulDNSNotificationValue = 0;

    xDNSResolutionTaskHandle = xTaskGetCurrentTaskHandle();

    if( strlen( pcHostName ) <= ( size_t ) SOCKETS_MAX_HOST_NAME_LENGTH )
    {
        xLwipError = dns_gethostbyname_addrtype( pcHostName, &xLwipIpv4Address,
                                                 lwip_dns_found_callback, ( void * ) &ulAddr,
                                                 LWIP_DNS_ADDRTYPE_IPV4 );

        switch( xLwipError )
        {
            case ERR_OK:
                ulAddr = *( ( uint32_t * ) &xLwipIpv4Address ); /* NOTE: IPv4 addresses only */
                break;

            case ERR_INPROGRESS:

                /*
                 * The DNS resolver is working the request.  Wait for it to complete
                 * or time out; print a timeout error message if configured for debug
                 * printing.
                 */
                ulDNSNotificationValue = ulTaskNotifyTake( pdTRUE, ( lwipdnsresolverMAX_WAIT_SECONDS * 1000 ) / portTICK_PERIOD_MS );
                xDNSResolutionTaskHandle = NULL;

                if( ( ulDNSNotificationValue != 1 ) || ( ulAddr == 0 ) )
                {
                    /* DNS resolution timed out */
                    configPRINTF( ( "Unable to resolve (%s) within (%lu) seconds",
                                    pcHostName, lwipdnsresolverMAX_WAIT_SECONDS ) );
                }

                break;

            default:
                configPRINTF( ( "Unexpected error (%lu) from dns_gethostbyname_addrtype() while resolving (%s)!",
                                ( uint32_t ) xLwipError, pcHostName ) );
                break;
        }
    }
    else
    {
        ulAddr = 0;
        configPRINTF( ( "Host name (%s) too long!", pcHostName ) );
    }

    return ulAddr;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Init()
{
    return SOCKETS_ERROR_NONE;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_DeInit()
{
    return SOCKETS_ERROR_NONE;
}
/*-----------------------------------------------------------*/

SocketHandle Sockets_Open()
{
    int32_t ulSocketNumber = lwip_socket( AF_INET, SOCK_STREAM, IP_PROTO_TCP );
    SocketHandle xSocket;

    if( ulSocketNumber < 0 )
    {
        xSocket = ( SocketHandle ) SOCKETS_INVALID_SOCKET;
    }
    else
    {
        xSocket = ( SocketHandle ) ulSocketNumber;
        UART_PRINT("SOCKET OPENED \r\n");
    }

    return xSocket;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Close( SocketHandle xSocket )
{
    return ( BaseType_t ) lwip_close( ( uint32_t ) xSocket );
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Connect( SocketHandle xSocket,
                            const char * pcHostName,
                            uint16_t usPort )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket;
    int32_t lRetVal = SOCKETS_ERROR_NONE;
    uint32_t ulIPAddres = 0;
    struct sockaddr_in xSockAddr = { 0 };

    ip_addr_t TargetIp;
    DNS_IF_init();

    if( DNS_IF_gethostbyname(pcHostName, &TargetIp)  != 0 )
    {
        lRetVal = SOCKETS_SOCKET_ERROR;
    }
    else
    {
        xSockAddr.sin_family = AF_INET;
        xSockAddr.sin_addr.s_addr = *( ( uint32_t * ) &TargetIp);
        xSockAddr.sin_len   = sizeof(xSockAddr);
        xSockAddr.sin_port = lwip_htons( usPort );

        /* Check for valid socket and connect */
        if (ulSocketNumber < 0) {
            lRetVal = SOCKETS_SOCKET_ERROR;
        }
        else
        {
            if( lwip_connect( ulSocketNumber, ( struct sockaddr * ) &xSockAddr, sizeof( xSockAddr ) ) < 0 )
            {
                lRetVal = SOCKETS_SOCKET_ERROR;
            }
            else
            {
                UART_PRINT("SOCKET CONNECTED \r\n");
            }
        }
    }

    return lRetVal;
}
/*-----------------------------------------------------------*/

void Sockets_Disconnect( SocketHandle xSocket )
{
    lwip_close( ( uint32_t ) xSocket );
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Recv( SocketHandle xSocket,
                         uint8_t * pucReceiveBuffer,
                         size_t xReceiveBufferLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket;
    int lRetVal = lwip_recv( ulSocketNumber,
                             pucReceiveBuffer,
                             xReceiveBufferLength,
                             0 );

    if( lRetVal == -1 )
    {
        /*
         * 1. EWOULDBLOCK if the socket is NON-blocking, but there is no data
         *    when recv is called.
         * 2. EAGAIN if the socket would block and have waited long enough but
         *    packet is not received.
         */
        if( ( errno == EWOULDBLOCK ) || ( errno == EAGAIN ) )
        {
            return SOCKETS_ERROR_NONE; /* timeout or would block */
        }

        /*
         * socket is not connected.
         */
        if( errno == EBADF )
        {
            return SOCKETS_ECLOSED;
        }
    }

    if( ( lRetVal == 0 ) && ( errno == ENOTCONN ) )
    {
        lRetVal = SOCKETS_ECLOSED;
    }

    return ( BaseType_t ) lRetVal;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Send( SocketHandle xSocket,
                         const uint8_t * pucData,
                         size_t xDataLength )
{
    return ( BaseType_t ) lwip_send( ( uint32_t ) xSocket,
                                     pucData,
                                     xDataLength,
                                     0 );
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_SetSockOpt( SocketHandle xSocket,
                               int32_t lOptionName,
                               const void * pvOptionValue,
                               size_t xOptionLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket;
    BaseType_t xRetVal;
    int ulRet = 0;

    switch( lOptionName )
    {
        case SOCKETS_SO_RCVTIMEO:
           {
               TickType_t xTicks;
               struct timeval xTV;

               xTicks = *( ( const TickType_t * ) pvOptionValue );

               xTV.tv_sec = TICK_TO_S( xTicks );
               xTV.tv_usec = TICK_TO_US( xTicks % configTICK_RATE_HZ );

               ulRet = lwip_setsockopt( ulSocketNumber,
                                        SOL_SOCKET,
                                        lOptionName == SOCKETS_SO_RCVTIMEO ?
                                        SO_RCVTIMEO : SO_SNDTIMEO,
                                        ( struct timeval * ) &xTV,
                                        sizeof( xTV ) );

               if( ulRet != 0 )
               {
                   xRetVal = SOCKETS_EINVAL;
               }
               else
               {
                   xRetVal = SOCKETS_ERROR_NONE;
               }
           }
           break;

        /*** Modified to accomodate LWIP library ***/
        case SOCKETS_SO_SNDTIMEO:
            xRetVal = SOCKETS_ERROR_NONE;
            break;

        default:
            xRetVal = SOCKETS_ENOPROTOOPT;
            break;
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/
