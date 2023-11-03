/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* RTOS header file */
#include "cyabs_rtos.h"

/* Cypress secure socket header file */
#include "cy_secure_sockets.h"

/* Wi-Fi connection manager header files */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* Standard C header file */
#include <string.h>

#include "sample_publisher_task.h"

/* IP address related header files. */
#include "cy_nw_helper.h"

/* Standard C header files */
#include <inttypes.h>

#define TCP_SERVER_TASK_STACK_SIZE (1024 * 5)
#define TCP_SERVER_TASK_PRIORITY (1)

/*******************************************************************************
 * Macros
 ********************************************************************************/
/* To use the Wi-Fi device in AP interface mode, set this macro as '1' */
#define USE_AP_INTERFACE (0)

#define MAKE_IP_PARAMETERS(a, b, c, d) ((((uint32_t)d) << 24) | \
                                        (((uint32_t)c) << 16) | \
                                        (((uint32_t)b) << 8) |  \
                                        ((uint32_t)a))

#define IP_ADDR_BUFFER_SIZE (20u)

#define WIFI_INTERFACE_TYPE CY_WCM_INTERFACE_TYPE_STA

/* Wi-Fi Credentials: Modify WIFI_SSID, WIFI_PASSWORD, and WIFI_SECURITY_TYPE
 * to match your Wi-Fi network credentials.
 * Note: Maximum length of the Wi-Fi SSID and password is set to
 * CY_WCM_MAX_SSID_LEN and CY_WCM_MAX_PASSPHRASE_LEN as defined in cy_wcm.h file.
 */
#define WIFI_SSID "stanton24"
#define WIFI_PASSWORD "B00BF00D77"

/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
 * in "cy_wcm.h" for more details.
 */
#define WIFI_SECURITY_TYPE CY_WCM_SECURITY_WPA2_AES_PSK
/* Maximum number of connection retries to a Wi-Fi network. */
#define MAX_WIFI_CONN_RETRIES (10u)

/* Wi-Fi re-connection time interval in milliseconds */
#define WIFI_CONN_RETRY_INTERVAL_MSEC (1000u)

/* TCP server related macros. */
#define TCP_SERVER_PORT (50007)
#define TCP_SERVER_MAX_PENDING_CONNECTIONS (3u)
#define TCP_SERVER_RECV_TIMEOUT_MS (500u)
#define MAX_TCP_RECV_BUFFER_SIZE (20u)

/* TCP keep alive related macros. */
#define TCP_KEEP_ALIVE_IDLE_TIME_MS (10000u)
#define TCP_KEEP_ALIVE_INTERVAL_MS (1000u)
#define TCP_KEEP_ALIVE_RETRY_COUNT (2u)

/* Length of the LED ON/OFF command issued from the TCP server. */
#define TCP_LED_CMD_LEN (1)
#define MEAS_BATCH_LEN sizeof(measurement_batch)

/* Debounce delay for user button. */
#define DEBOUNCE_DELAY_MS (50)

extern cy_queue_t meas_q;
/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
static cy_rslt_t create_tcp_server_socket(void);
static cy_rslt_t tcp_connection_handler(cy_socket_t socket_handle, void *arg);
static cy_rslt_t tcp_receive_msg_handler(cy_socket_t socket_handle, void *arg);
static cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg);

static cy_rslt_t connect_to_wifi_ap(void);

/*******************************************************************************
 * Global Variables
 ********************************************************************************/
/* Secure socket variables. */
cy_socket_sockaddr_t tcp_server_addr, peer_addr;
cy_socket_t server_handle, client_handle;

/* Size of the peer socket address. */
uint32_t peer_addr_len;

/* Flags to track the LED state. */
bool led_state = CYBSP_LED_STATE_OFF;

/* Flag variable to check if TCP client is connected. */
bool client_connected;

/*******************************************************************************
 * Function Name: sample_publisher_task
 *******************************************************************************
 * Summary:
 *  Task used to establish a connection to a TCP client.
 *
 * Parameters:
 *  void *args : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void sample_publisher_task(void *arg)
{
    init_wifi();
    cyhal_gpio_toggle(CYBSP_USER_LED);
    cy_rslt_t result;

    measurement_batch meas_batch;
    /* Variable to store number of bytes sent over TCP socket. */
    uint32_t bytes_sent = 0;
    while (true)
    {
        if (client_connected)
        {
            result = cy_rtos_get_queue(&meas_q, &meas_batch, 1, false);
            if (result == CY_RSLT_SUCCESS)
            {
                result = cy_socket_send(client_handle, &meas_batch, MEAS_BATCH_LEN,
                                        CY_SOCKET_FLAGS_NONE, &bytes_sent);
                if (result == CY_RSLT_SUCCESS)
                {
                    // TODO?
                }
                else
                {
                    printf("Failed to send command to client. Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
                    if (result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED)
                    {
                        /* Disconnect the socket. */
                        cy_socket_disconnect(client_handle, 0);
                        /* Delete the socket. */
                        cy_socket_delete(client_handle);
                    }
                }
            }
            /* Send the command to TCP client. */
        }
        vTaskDelay(5);
    }
}

#if (!USE_AP_INTERFACE)
/*******************************************************************************
 * Function Name: connect_to_wifi_ap()
 *******************************************************************************
 * Summary:
 *  Connects to Wi-Fi AP using the user-configured credentials, retries up to a
 *  configured number of times until the connection succeeds.
 *
 *******************************************************************************/
static cy_rslt_t connect_to_wifi_ap(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    char ip_addr_str[IP_ADDR_BUFFER_SIZE];

    /* Variables used by Wi-Fi connection manager.*/
    cy_wcm_connect_params_t wifi_conn_param;

    cy_wcm_ip_address_t ip_address;

    /* IP variable for network utility functions */
    cy_nw_ip_address_t nw_ip_addr =
        {
            .version = NW_IP_IPV4};

    /* Variable to track the number of connection retries to the Wi-Fi AP specified
     * by WIFI_SSID macro.
     */
    int conn_retries = 0;

    /* Set the Wi-Fi SSID, password and security type. */
    memset(&wifi_conn_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(wifi_conn_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(wifi_conn_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    wifi_conn_param.ap_credentials.security = WIFI_SECURITY_TYPE;

    printf("Connecting to Wi-Fi Network: %s\n", WIFI_SSID);

    /* Join the Wi-Fi AP. */
    for (conn_retries = 0; conn_retries < MAX_WIFI_CONN_RETRIES; conn_retries++)
    {
        result = cy_wcm_connect_ap(&wifi_conn_param, &ip_address);

        if (result == CY_RSLT_SUCCESS)
        {
            printf("Successfully connected to Wi-Fi network '%s'.\n",
                   wifi_conn_param.ap_credentials.SSID);
            nw_ip_addr.ip.v4 = ip_address.ip.v4;
            cy_nw_ntoa(&nw_ip_addr, ip_addr_str);
            printf("IP Address Assigned: %s\n", ip_addr_str);

            /* IP address and TCP port number of the TCP server */
            tcp_server_addr.ip_address.ip.v4 = ip_address.ip.v4;
            tcp_server_addr.ip_address.version = CY_SOCKET_IP_VER_V4;
            tcp_server_addr.port = TCP_SERVER_PORT;
            return result;
        }

        printf("Connection to Wi-Fi network failed with error code 0x%08" PRIx32 "\n."
               "Retrying in %d ms...\n",
               (uint32_t)result, WIFI_CONN_RETRY_INTERVAL_MSEC);
        cy_rtos_delay_milliseconds(WIFI_CONN_RETRY_INTERVAL_MSEC);
    }

    /* Stop retrying after maximum retry attempts. */
    printf("Exceeded maximum Wi-Fi connection attempts\n");

    return result;
}
#endif /* USE_AP_INTERFACE */

/*******************************************************************************
 * Function Name: create_tcp_server_socket
 *******************************************************************************
 * Summary:
 *  Function to create a socket and set the socket options
 *
 *******************************************************************************/
static cy_rslt_t create_tcp_server_socket(void)
{
    cy_rslt_t result;
    /* TCP socket receive timeout period. */
    uint32_t tcp_recv_timeout = TCP_SERVER_RECV_TIMEOUT_MS;

    /* Variables used to set socket options. */
    cy_socket_opt_callback_t tcp_receive_option;
    cy_socket_opt_callback_t tcp_connection_option;
    cy_socket_opt_callback_t tcp_disconnection_option;

    /* Create a TCP socket */
    result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM,
                              CY_SOCKET_IPPROTO_TCP, &server_handle);
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Failed to create socket! Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
        return result;
    }

    /* Set the TCP socket receive timeout period. */
    result = cy_socket_setsockopt(server_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_RCVTIMEO, &tcp_recv_timeout,
                                  sizeof(tcp_recv_timeout));
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_RCVTIMEO failed\n");
        return result;
    }

    /* Register the callback function to handle connection request from a TCP client. */
    tcp_connection_option.callback = tcp_connection_handler;
    tcp_connection_option.arg = NULL;

    result = cy_socket_setsockopt(server_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_CONNECT_REQUEST_CALLBACK,
                                  &tcp_connection_option, sizeof(cy_socket_opt_callback_t));
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_CONNECT_REQUEST_CALLBACK failed\n");
        return result;
    }

    /* Register the callback function to handle messages received from a TCP client. */
    tcp_receive_option.callback = tcp_receive_msg_handler;
    tcp_receive_option.arg = NULL;

    result = cy_socket_setsockopt(server_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_RECEIVE_CALLBACK,
                                  &tcp_receive_option, sizeof(cy_socket_opt_callback_t));
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_RECEIVE_CALLBACK failed\n");
        return result;
    }

    /* Register the callback function to handle disconnection. */
    tcp_disconnection_option.callback = tcp_disconnection_handler;
    tcp_disconnection_option.arg = NULL;

    result = cy_socket_setsockopt(server_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_DISCONNECT_CALLBACK,
                                  &tcp_disconnection_option, sizeof(cy_socket_opt_callback_t));
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_DISCONNECT_CALLBACK failed\n");
        return result;
    }

    /* Bind the TCP socket created to Server IP address and to TCP port. */
    result = cy_socket_bind(server_handle, &tcp_server_addr, sizeof(tcp_server_addr));
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Failed to bind to socket! Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
    }

    return result;
}

/*******************************************************************************
 * Function Name: tcp_connection_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle incoming TCP client connection.
 *
 * Parameters:
 * cy_socket_t socket_handle: Connection handle for the TCP server socket
 *  void *args : Parameter passed on to the function (unused)
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
static cy_rslt_t tcp_connection_handler(cy_socket_t socket_handle, void *arg)
{
    cy_rslt_t result;
    char ip_addr_str[IP_ADDR_BUFFER_SIZE];

    /* IP variable for network utility functions */
    cy_nw_ip_address_t nw_ip_addr =
        {
            .version = NW_IP_IPV4};

    /* TCP keep alive parameters. */
    int keep_alive = 1;
#if defined(COMPONENT_LWIP)
    uint32_t keep_alive_interval = TCP_KEEP_ALIVE_INTERVAL_MS;
    uint32_t keep_alive_count = TCP_KEEP_ALIVE_RETRY_COUNT;
    uint32_t keep_alive_idle_time = TCP_KEEP_ALIVE_IDLE_TIME_MS;
#endif

    /* Accept new incoming connection from a TCP client.*/
    result = cy_socket_accept(socket_handle, &peer_addr, &peer_addr_len,
                              &client_handle);
    if (result == CY_RSLT_SUCCESS)
    {
        printf("Incoming TCP connection accepted\n");
        nw_ip_addr.ip.v4 = peer_addr.ip_address.ip.v4;
        cy_nw_ntoa(&nw_ip_addr, ip_addr_str);
        printf("IP Address : %s\n\n", ip_addr_str);
        printf("Press the user button to send LED ON/OFF command to the TCP client\n");

#if defined(COMPONENT_LWIP)
        /* Set the TCP keep alive interval. */
        result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_TCP,
                                      CY_SOCKET_SO_TCP_KEEPALIVE_INTERVAL,
                                      &keep_alive_interval, sizeof(keep_alive_interval));
        if (result != CY_RSLT_SUCCESS)
        {
            printf("Set socket option: CY_SOCKET_SO_TCP_KEEPALIVE_INTERVAL failed\n");
            return result;
        }

        /* Set the retry count for TCP keep alive packet. */
        result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_TCP,
                                      CY_SOCKET_SO_TCP_KEEPALIVE_COUNT,
                                      &keep_alive_count, sizeof(keep_alive_count));
        if (result != CY_RSLT_SUCCESS)
        {
            printf("Set socket option: CY_SOCKET_SO_TCP_KEEPALIVE_COUNT failed\n");
            return result;
        }

        /* Set the network idle time before sending the TCP keep alive packet. */
        result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_TCP,
                                      CY_SOCKET_SO_TCP_KEEPALIVE_IDLE_TIME,
                                      &keep_alive_idle_time, sizeof(keep_alive_idle_time));
        if (result != CY_RSLT_SUCCESS)
        {
            printf("Set socket option: CY_SOCKET_SO_TCP_KEEPALIVE_IDLE_TIME failed\n");
            return result;
        }
#endif

        /* Enable TCP keep alive. */
        result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_SOCKET,
                                      CY_SOCKET_SO_TCP_KEEPALIVE_ENABLE,
                                      &keep_alive, sizeof(keep_alive));
        if (result != CY_RSLT_SUCCESS)
        {
            printf("Set socket option: CY_SOCKET_SO_TCP_KEEPALIVE_ENABLE failed\n");
            return result;
        }

        /* Set the client connection flag as true. */
        client_connected = true;
    }
    else
    {
        printf("Failed to accept incoming client connection. Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
        printf("===============================================================\n");
        printf("Listening for incoming TCP client connection on Port: %d\n",
               tcp_server_addr.port);
    }

    return result;
}

/*******************************************************************************
 * Function Name: tcp_receive_msg_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle incoming TCP client messages.
 *
 * Parameters:
 * cy_socket_t socket_handle: Connection handle for the TCP client socket
 *  void *args : Parameter passed on to the function (unused)
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
static cy_rslt_t tcp_receive_msg_handler(cy_socket_t socket_handle, void *arg)
{
    char message_buffer[MAX_TCP_RECV_BUFFER_SIZE];
    cy_rslt_t result;

    /* Variable to store number of bytes received from TCP client. */
    uint32_t bytes_received = 0;
    result = cy_socket_recv(socket_handle, message_buffer, MAX_TCP_RECV_BUFFER_SIZE,
                            CY_SOCKET_FLAGS_NONE, &bytes_received);

    if (result == CY_RSLT_SUCCESS)
    {
        /* Terminate the received string with '\0'. */
        message_buffer[bytes_received] = '\0';
        printf("\r\nAcknowledgement from TCP Client: %s\n", message_buffer);
    }
    else
    {
        printf("Failed to receive acknowledgement from the TCP client. Error: 0x%08" PRIx32 "\n",
               (uint32_t)result);
        if (result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED)
        {
            /* Disconnect the socket. */
            cy_socket_disconnect(socket_handle, 0);
            /* Delete the socket. */
            cy_socket_delete(socket_handle);
        }
    }

    printf("===============================================================\n");
    printf("Press the user button to send LED ON/OFF command to the TCP client\n");

    return result;
}

/*******************************************************************************
 * Function Name: tcp_disconnection_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle TCP client disconnection event.
 *
 * Parameters:
 * cy_socket_t socket_handle: Connection handle for the TCP client socket
 *  void *args : Parameter passed on to the function (unused)
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
static cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg)
{
    cy_rslt_t result;

    /* Disconnect the TCP client. */
    result = cy_socket_disconnect(socket_handle, 0);
    /* Delete the socket. */
    cy_socket_delete(socket_handle);

    /* Set the client connection flag as false. */
    client_connected = false;
    printf("TCP Client disconnected! Please reconnect the TCP Client\n");
    printf("===============================================================\n");
    printf("Listening for incoming TCP client connection on Port:%d\n",
           tcp_server_addr.port);

    /* Set the LED state to OFF when the TCP client disconnects. */
    led_state = CYBSP_LED_STATE_OFF;

    return result;
}

cy_rslt_t init_wifi()
{
    cy_rslt_t result = 0;
    cy_wcm_config_t wifi_config = {.interface = WIFI_INTERFACE_TYPE};

    /* Initialize Wi-Fi connection manager. */
    result = cy_wcm_init(&wifi_config);

    if (result != CY_RSLT_SUCCESS)
    {
        printf("Wi-Fi Connection Manager initialization failed! Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
        CY_ASSERT(0);
    }
    printf("Wi-Fi Connection Manager initialized.\r\n");

    /* Connect to Wi-Fi AP */
    result = connect_to_wifi_ap();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("\n Failed to connect to Wi-Fi AP! Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
        CY_ASSERT(0);
    }

    /* Initialize secure socket library. */
    result = cy_socket_init();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Secure Socket initialization failed! Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
        CY_ASSERT(0);
    }
    printf("Secure Socket initialized\n");

    /* Create TCP server socket. */
    result = create_tcp_server_socket();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Failed to create socket! Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
        CY_ASSERT(0);
    }

    /* Start listening on the TCP server socket. */
    result = cy_socket_listen(server_handle, TCP_SERVER_MAX_PENDING_CONNECTIONS);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_socket_delete(server_handle);
        printf("cy_socket_listen returned error. Error code: 0x%08" PRIx32 "\n", (uint32_t)result);
        CY_ASSERT(0);
    }
    else
    {
        printf("===============================================================\n");
        printf("Listening for incoming TCP client connection on Port: %d\n",
               tcp_server_addr.port);
    }
    return result;
}

cy_rslt_t create_sample_publisher_task()
{
    BaseType_t status;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the User LED */
    result = cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT,
                             CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    status = xTaskCreate(sample_publisher_task, "Network task", TCP_SERVER_TASK_STACK_SIZE, NULL,
                         TCP_SERVER_TASK_PRIORITY, NULL);
    return (status == pdPASS) ? CY_RSLT_SUCCESS : (cy_rslt_t)status;
}
