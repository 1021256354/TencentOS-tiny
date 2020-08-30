#include "stm32f1xx_hal.h"
#include "mcu_init.h"
#include "tos_k.h"
#include "mqttclient.h"
#include "cjson.h"
#include "sal_module_wrapper.h"
#include "ds18b20.h"

//#define USE_ESP8266
//#define USE_NB_BC35
//#define USE_BC26
#define USE_EC200S

#if defined(USE_ESP8266)
#include "esp8266.h"
#elif defined(USE_BC26)
#include "bc26.h"
#elif defined(USE_EC200S)
#include "ec200s.h"
#endif

#ifdef USE_ESP8266 
static hal_uart_port_t esp8266_port = HAL_UART_PORT_0;

void mqtt_set_esp8266_port(hal_uart_port_t port) {
    esp8266_port = port;
}
#endif

k_event_t report_result_event;
k_event_flag_t report_success = 1<<0;
k_event_flag_t report_fail    = 1<<1;

#define REPORT_DATA_TEMPLATE "{\"method\":\"report\",\"clientToken\":\"00000001\",\"params\":{\"TempValue\":%d.%-2d}}"
#define CONTROL_REPLY_DATA   "{\"method\":\"control_reply\",\"clientToken\":\"%s\",\"code\":0,\"status\":\"success\"}"

char report_buf[200];
char reply_buf[200];

static void tos_topic_handler(void* client, message_data_t* msg)
{
    (void) client;
    cJSON* cjson_root   = NULL;
    cJSON* cjson_status = NULL;
    cJSON* cjson_method = NULL;
    cJSON* cjson_params = NULL;
    cJSON* cjson_relay_status = NULL;
    cJSON* cjson_client_token = NULL;
    char*  status = NULL;
    char*  method = NULL;
    char*  client_token = NULL;
    int    relay_status = 0;
    k_event_flag_t event_flag = report_fail;
    mqtt_message_t reply_msg;
    int error;

    /* ��ӡ��־ */
    MQTT_LOG_I("-----------------------------------------------------------------------------------");
    MQTT_LOG_I("%s:%d %s()...\ntopic: %s, qos: %d. \nmessage:\n\t%s\n", __FILE__, __LINE__, __FUNCTION__, 
            msg->topic_name, msg->message->qos, (char*)msg->message->payload);
    MQTT_LOG_I("-----------------------------------------------------------------------------------\n");
    
    /* ʹ��cjson�����ϱ���Ӧ���� */
    cjson_root = cJSON_Parse((char*)msg->message->payload);
    if (cjson_root == NULL) {
        printf("subscribe message parser fail\r\n");
        event_flag = report_fail;
        goto exit;
    }
    
    /* ��ȡ��Ϣ���� */
    cjson_method = cJSON_GetObjectItem(cjson_root, "method");
    method = cJSON_GetStringValue(cjson_method);
    
    /* �ж����������͵���Ϣ */
    if (strstr(method, "report_reply")) {
        
        //�����ϱ���Ӧ���ģ���ȡstatus״ֵ̬
        cjson_status = cJSON_GetObjectItem(cjson_root, "status");
        status = cJSON_GetStringValue(cjson_status);
        if (cjson_status == NULL || status == NULL) {
            printf("report reply status parser fail\r\n");
            event_flag = report_fail;
            goto exit;
        }
        
        //�ж�status״̬
        if (strstr(status,"success")) {
            event_flag = report_success;
        }else {
            event_flag = report_fail;
        }   
    } else if (strstr(method, "control")) {
    
        //�յ�ƽ̨�·��Ŀ��Ʊ��ģ���ȡclient_token�������ϱ���Ӧ
        cjson_client_token = cJSON_GetObjectItem(cjson_root, "clientToken");
        client_token = cJSON_GetStringValue(cjson_client_token);
        
        printf("parse client token:%s\r\n", client_token);
        
        //��ȡ relay_status
        cjson_params = cJSON_GetObjectItem(cjson_root, "params");
        cjson_relay_status = cJSON_GetObjectItem(cjson_params, "relay_status");
        relay_status = cjson_relay_status->valueint;
        
        //���� relay_status ִ����Ӧ�Ķ���
        if (relay_status == 0) {
            HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_SET);
        } else if (relay_status == 1) {
            HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET);
        }
        
        memset(reply_buf, 0, sizeof(reply_buf));
        sprintf(reply_buf, CONTROL_REPLY_DATA, client_token);
        memset(&reply_msg, 0, sizeof(reply_msg));
        reply_msg.qos = QOS0;
        reply_msg.payload = (void *) reply_buf;
        
        printf("control reply:\r\n\t%s\r\n", reply_buf);
        
        error = mqtt_publish(client, "$thing/up/property/E2IGF491FP/dev001", &reply_msg);
        
        MQTT_LOG_D("control reply publish error is %#0x", error);

        cJSON_Delete(cjson_root);
        cjson_root = NULL;
        method = NULL;
        client_token = NULL;
        relay_status = 0;
        
        return;
    }

exit:
    cJSON_Delete(cjson_root);
    cjson_root = NULL;
    status = NULL;
    
    tos_event_post(&report_result_event, event_flag);
    
    return;
}

void mqttclient_task(void)
{
    int error;
    
    uint16_t temp_value = 0;
    int intT = 0, decT = 0;
    
    mqtt_client_t *client = NULL;
    
    mqtt_message_t msg;
    
    k_event_flag_t match_flag;
    
    char  host_ip[20];
    
    memset(&msg, 0, sizeof(msg));
    
    /* �ر� DTU ��ɫָʾ�� */
    HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
    
    /* �رռ̵��� */
    HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_SET);
    
    /* ��ʼ��DS18B20 */
    DS18B20_Init();
    
#ifdef USE_ESP8266 
    esp8266_sal_init(esp8266_port);
    esp8266_join_ap("Supowang", "13975428888");
#endif

#ifdef USE_NB_BC35
    int bc35_28_95_sal_init(hal_uart_port_t uart_port);
    bc35_28_95_sal_init(HAL_UART_PORT_0);
#endif

#ifdef USE_BC26
    bc26_sal_init(HAL_UART_PORT_2);
#endif

#ifdef USE_EC200S
    ec200s_sal_init(HAL_UART_PORT_2);
#endif

    mqtt_log_init();

    client = mqtt_lease();
    
    tos_event_create(&report_result_event, (k_event_flag_t)0u);
    
    /* Domain Format: <your product ID>.iotcloud.tencentdevices.com */
    tos_sal_module_parse_domain("E2IGF491FP.iotcloud.tencentdevices.com",host_ip,sizeof(host_ip));
    
    /*
        These infomation is generated by mqtt_config_gen.py tool in "TencentOS-tiny\tools" directory.
    */
    mqtt_set_port(client, "1883");
    mqtt_set_host(client, host_ip);
    mqtt_set_client_id(client, "E2IGF491FPdev001");
    mqtt_set_user_name(client, "E2IGF491FPdev001;21010406;12365;4294967295");
    mqtt_set_password(client, "b69beb5ded277e41a1f38ba560d9ff203d56f3f8;hmacsha1");
    mqtt_set_clean_session(client, 1);

    error = mqtt_connect(client);
    
    MQTT_LOG_D("mqtt connect error is %#0x", error);
    
    /* ���� DTU ��ɫָʾ�� */
    if (error == 0) {
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
    }
    
    error = mqtt_subscribe(client, "$thing/down/property/E2IGF491FP/dev001", QOS0, tos_topic_handler);
    
    MQTT_LOG_D("mqtt subscribe error is %#0x", error);
    
    while (1) {
        
        memset(&msg, 0, sizeof(msg));
        temp_value = DS18B20_Read_Temperature();
        intT   = temp_value >> 4 ;
        decT   = temp_value & 0x0F;
        memset(report_buf, 0, sizeof(report_buf));
        snprintf(report_buf, sizeof(report_buf), REPORT_DATA_TEMPLATE, intT, decT);
        printf("DS18B20:%d.%-2d\r\n", intT, decT);
       
        msg.qos = QOS0;
        msg.payload = (void *) report_buf;
        
        error = mqtt_publish(client, "$thing/up/property/E2IGF491FP/dev001", &msg);
        
        MQTT_LOG_D("data report publish error is %#0x", error);
        
        tos_event_pend(&report_result_event, 
                       report_success|report_fail,
                       &match_flag,
                       TOS_TIME_FOREVER,
                       TOS_OPT_EVENT_PEND_ANY | TOS_OPT_EVENT_PEND_CLR);
        
        if (match_flag == report_success) {
            printf("report to Tencent IoT Explorer success\r\n");
        
        }else if (match_flag == report_fail){
            printf("report to Tencent IoT Explorer fail\r\n");
        }
        
        tos_task_delay(5000); 
    }
}

void application_entry(void *arg)
{
    mqttclient_task();
    while (1) {
        printf("This is a mqtt demo!\r\n");
        tos_task_delay(1000);
    }
}
