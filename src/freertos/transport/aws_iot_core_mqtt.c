/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file aws_iot_core_mqtt.c
 * @brief Implements the port for AWS IoT MQTT based on coreMQTT.
 *
 * @note This interface is private and subjected to change. Currently, there is only
 *       one implementation for this interface, which uses coreMQTT as underlying MQTT stack.
 *
 */

#include <assert.h>

#include "aws_iot_mqtt.h"

#include "core_mqtt_serializer.h"


#include "core_mqtt.h"

/**
 * @brief The length of the outgoing publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for outgoing publishes.
 * Number of publishes = ulMaxPublishCount * mqttexampleTOPIC_COUNT
 * Update in ulMaxPublishCount needs updating mqttexampleOUTGOING_PUBLISH_RECORD_LEN.
 */
#define mqttexampleOUTGOING_PUBLISH_RECORD_LEN            ( 15U )

/**
 * @brief The length of the incoming publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for incoming publishes.
 * Number of publishes = ulMaxPublishCount * mqttexampleTOPIC_COUNT
 * Update in ulMaxPublishCount needs updating mqttexampleINCOMING_PUBLISH_RECORD_LEN.
 */
#define mqttexampleINCOMING_PUBLISH_RECORD_LEN            ( 15U )

/**
 * @brief Array to track the outgoing publish records for outgoing publishes
 * with QoS > 0.
 *
 * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
 *
 */
static MQTTPubAckInfo_t pOutgoingPublishRecords[ mqttexampleOUTGOING_PUBLISH_RECORD_LEN ];

/**
 * @brief Array to track the incoming publish records for incoming publishes
 * with QoS > 0.
 *
 * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
 *
 */
static MQTTPubAckInfo_t pIncomingPublishRecords[ mqttexampleINCOMING_PUBLISH_RECORD_LEN ];


MQTTStatus_t AwsIoTMQTT_Init( MQTTContext_t * xContext,
                                const TransportInterface_t * pxTransportInterface,
                                MQTTGetCurrentTimeFunc_t xGetTimeFunction,
                                MQTTEventCallback_t xUserCallback,
                                uint8_t * pucNetworkBuffer,
                                size_t xNetworkBufferLength )
{
    MQTTFixedBuffer_t xBuffer = { pucNetworkBuffer, xNetworkBufferLength };
    MQTTStatus_t xResult;

    xResult = MQTT_Init( xContext,
                         ( const TransportInterface_t * ) pxTransportInterface,
                         ( MQTTGetCurrentTimeFunc_t ) xGetTimeFunction,
                         ( MQTTEventCallback_t ) xUserCallback,
                         &xBuffer );
    
    if (xResult == MQTTSuccess)
    {
        xResult = MQTT_InitStatefulQoS( xContext,
                                        pOutgoingPublishRecords,
                                        mqttexampleOUTGOING_PUBLISH_RECORD_LEN,
                                        pIncomingPublishRecords,
                                        mqttexampleINCOMING_PUBLISH_RECORD_LEN );
    }

    return ( xResult );
}

MQTTStatus_t AwsIoTMQTT_Connect( MQTTContext_t * xContext,
                                           const MQTTConnectInfo_t * pxConnectInfo,
                                           const MQTTPublishInfo_t * pxWillInfo,
                                           uint32_t ulMilliseconds,
                                           bool * pxSessionPresent )
{
    MQTTStatus_t xResult;

    xResult = MQTT_Connect( xContext,
                            ( const MQTTConnectInfo_t * ) pxConnectInfo,
                            ( const MQTTPublishInfo_t * ) pxWillInfo,
                            ulMilliseconds, pxSessionPresent );

    return ( xResult );
}

MQTTStatus_t AwsIoTMQTT_Subscribe( MQTTContext_t * xContext,
                                             const MQTTSubscribeInfo_t * pxSubscriptionList,
                                             size_t xSubscriptionCount,
                                             uint16_t usPacketId )
{
    MQTTStatus_t xResult;

    xResult = MQTT_Subscribe( xContext, ( const MQTTSubscribeInfo_t * ) pxSubscriptionList,
                              xSubscriptionCount, usPacketId );

    return ( xResult );
}

MQTTStatus_t AwsIoTMQTT_Publish( MQTTContext_t * xContext,
                                           const MQTTPublishInfo_t * pxPublishInfo,
                                           uint16_t usPacketId )
{
    MQTTStatus_t xResult;

    xResult = MQTT_Publish( xContext, ( const MQTTPublishInfo_t * ) pxPublishInfo,
                            usPacketId );

    return ( xResult );
}

MQTTStatus_t AwsIoTMQTT_Ping( MQTTContext_t * xContext )
{
    MQTTStatus_t xResult;

    xResult = MQTT_Ping( xContext );

    return ( xResult );
}

MQTTStatus_t AwsIoTMQTT_Unsubscribe( MQTTContext_t * xContext,
                                        const MQTTSubscribeInfo_t * pxSubscriptionList,
                                        size_t xSubscriptionCount,
                                        uint16_t usPacketId )
{
    MQTTStatus_t xResult;

    xResult = MQTT_Unsubscribe( xContext, ( const MQTTSubscribeInfo_t * ) pxSubscriptionList,
                                xSubscriptionCount, usPacketId );

    return ( xResult );
}

MQTTStatus_t AwsIoTMQTT_Disconnect( MQTTContext_t * xContext )
{
    MQTTStatus_t xResult;

    xResult = MQTT_Disconnect( xContext );

    return ( xResult );
}

MQTTStatus_t AwsIoTMQTT_ProcessLoop( MQTTContext_t * xContext,
                                        uint32_t ulMilliseconds )
{
    MQTTStatus_t xResult;

    xResult = MQTT_ProcessLoop( xContext );

    return ( xResult );
}

uint16_t AwsIoTMQTT_GetPacketId( MQTTContext_t * xContext )
{
    return MQTT_GetPacketId( xContext );
}

MQTTStatus_t AwsIoTMQTT_GetSubAckStatusCodes( const MQTTPacketInfo_t * pxSubackPacket,
                                                        uint8_t ** ppucPayloadStart,
                                                        size_t * pxPayloadSize )
{
    MQTTStatus_t xResult;

    xResult = MQTT_GetSubAckStatusCodes( ( const MQTTPacketInfo_t * ) pxSubackPacket,
                                         ppucPayloadStart, pxPayloadSize );

    return ( xResult );
}
