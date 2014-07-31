//! robotkernel module interface definition
/*!
 * author: Robert Burger
 *
 * $Id$
 */

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with robotkernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MODULE_INTF_H__
#define __MODULE_INTF_H__

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>

#define MODULE_HANDLE void*

typedef enum module_state {
    module_state_unknown = -2,
    module_state_error = -1,
    module_state_boot = 0,
    module_state_init = 1,
    module_state_preop = 2,
    module_state_safeop = 3,
    module_state_op = 4
} module_state_t;

#define __MOD_REQUEST(m, r, s)          (((s) << 24) | ((m) << 16) | (r))
#define __MOD_REQUEST_TYPE(s)           (sizeof(s))

#define MOD_REQUEST_MAGIC               0x14
#define MOD_REQUEST(x, s)               \
    __MOD_REQUEST((MOD_REQUEST_MAGIC), (x), __MOD_REQUEST_TYPE(s))

#define MOD_REQUEST_REGISTER_SERVICES   MOD_REQUEST(0x000B, void*)
#define MOD_REQUEST_SLAVE_ID2ADDRESS    MOD_REQUEST(0x000C, int)

// -----------------------------------------------------------------------------
// get process data pointers
//
typedef struct process_data {
    uint32_t slave_id; //! [in]     device slave_id
    void *pd;          //! [in/out] process data pointer
    size_t len;        //! [in/out] process data length
} process_data_t;

#define MOD_REQUEST_GET_PDIN            MOD_REQUEST(0x0001, process_data_t)
#define MOD_REQUEST_GET_PDOUT           MOD_REQUEST(0x0002, process_data_t)
#define MOD_REQUEST_GET_PD_COOKIE       MOD_REQUEST(0x000A, uint64_t *)

// -----------------------------------------------------------------------------
// set pd, used for double/multiple buffering of pd values

//! set pd
typedef struct set_pd {
    uint64_t pd_cookie; //! [in]     process data cookie
    uint32_t cnt;       //! [in]     number of indices
    process_data_t *pd; //! [in/out] field with indices
} set_pd_t;

#define MOD_REQUEST_SET_PDIN            MOD_REQUEST(0x0003, set_pd_t)
#define MOD_REQUEST_SET_PDOUT           MOD_REQUEST(0x0004, set_pd_t)

// ----------------------------------------------------------------------------
// trigger callbacks

typedef struct set_trigger_cb {
    void (*cb)(MODULE_HANDLE hdl);
    MODULE_HANDLE hdl;
    int clk_id;
} set_trigger_cb_t;
typedef std::list<set_trigger_cb_t> cb_list_t;

#define MOD_REQUEST_SET_TRIGGER_CB      MOD_REQUEST(0x0007, set_trigger_cb_t)
#define MOD_REQUEST_UNSET_TRIGGER_CB    MOD_REQUEST(0x0008, set_trigger_cb_t)
#define MOD_REQUEST_GET_CLOCKSRC_CNT    MOD_REQUEST(0x0009, int)

// ----------------------------------------------------------------------------
// sercos specific section

//! direction of service transfers
typedef enum sercos_service_direction {
    SSD_MASTER_TO_DRIVE = 0,      //!< data download to drive
    SSD_DRIVE_TO_MASTER = 1       //!< data upload from drive
} sercos_service_direction_t;

//! defines for element codes */
typedef enum sercos_service_element {
    SSE_NONE   = 0x00,    //!< place holder for element
    SSE_STRC   = 0x02,    //!< id structure
    SSE_NAME   = 0x04,    //!< name element of sercos id
    SSE_ATTR   = 0x08,    //!< attribute of sercos id
    SSE_UNIT   = 0x10,    //!< unit identifier of sercos id
    SSE_MINVAL = 0x20,    //!< minimum value of sercos id data
    SSE_MAXVAL = 0x40,    //!< maximum value of sercos id data
    SSE_DATA   = 0x80     //!< sercos id data
} sercos_service_element_t;

typedef enum sercos_service_attribute_datalength {
    SSA_DATALENGTH_RESERVED   = 0x0,
    SSA_DATALENGTH_2BYTEFIX   = 0x1,   //!< 2 byte fixed width
    SSA_DATALENGTH_4BYTEFIX   = 0x2,   //!< 4 byte fixed width
    SSA_DATALENGTH_8BYTEFIX   = 0x3,   //!< 8 byte fixed width
    SSA_DATALENGTH_1BYTEVAR   = 0x4,   //!< variable width with one byte length
    SSA_DATALENGTH_2BYTEVAR   = 0x5,   //!< variable width with two byte length
    SSA_DATALENGTH_4BYTEVAR   = 0x6,   //!< variable width with four byte length
    SSA_DATALENGTH_8BYTEVAR   = 0x7    //!< variable width with eight byte length
} sercos_service_attribute_datalength_t;

typedef enum sercos_service_attribute_datatype {
    SSA_DATATYPE_BINARY   = 0x0,
    SSA_DATATYPE_UINT     = 0x1,
    SSA_DATATYPE_INT      = 0x2,
    SSA_DATATYPE_UINT2    = 0x3,
    SSA_DATATYPE_CHARSET  = 0x4,
    SSA_DATATYPE_UINT3    = 0x5,
    SSA_DATATYPE_FLOAT    = 0x6,
    SSA_DATATYPE_RESERVED = 0x7
} sercos_service_attribute_datatype_t;

typedef struct sercos_service_attribute {
    uint16_t conversionfactor;  
    unsigned datalength   : 3;  
    unsigned function     : 1;
    unsigned datatype     : 3;
    unsigned reserved1    : 1;
    unsigned decimalpoint : 4;
    unsigned wp_cp2       : 1;
    unsigned wp_cp3       : 1;
    unsigned wp_cp4       : 1;
    unsigned reserved2    : 1;
} sercos_service_attribute_t;

//! sercos protocol specific service transfer
typedef struct sercos_service_transfer {
    int slave_id;                           //! slave id

    uint16_t idn;                           //! id number
    sercos_service_element_t element;       //! element numbers
    sercos_service_direction_t direction;   //! direction (to master or to slave)

    size_t buflen;
    uint16_t *buf;

    uint16_t scerror;
    uint16_t receipt;
    int change;

    sercos_service_transfer();
    sercos_service_transfer(uint32_t slave_id, uint16_t idn);
} sercos_service_transfer_t;

//! sercos execute procedure command on drive
typedef struct sercos_set_command {
    uint32_t slave_id;   //! at number
    uint16_t cmd;        //! procedure command
} sercos_set_command_t;

#define MOD_REQUEST_SERCOS_SERVICE_TRANSFER  MOD_REQUEST(0x0100, sercos_service_transfer_t)
#define MOD_REQUEST_SERCOS_GET_BAUDRATE      MOD_REQUEST(0x0101, int)
#define MOD_REQUEST_SERCOS_SET_BAUDRATE      MOD_REQUEST(0x0102, int)
#define MOD_REQUEST_SERCOS_GET_CYCLETIME     MOD_REQUEST(0x0103, int)
#define MOD_REQUEST_SERCOS_SET_CYCLETIME     MOD_REQUEST(0x0104, int)
#define MOD_REQUEST_SERCOS_GET_PHASE         MOD_REQUEST(0x0105, int)
#define MOD_REQUEST_SERCOS_SET_PHASE         MOD_REQUEST(0x0106, int)
#define MOD_REQUEST_SERCOS_GET_MASTERCLOCK   MOD_REQUEST(0x0107, int)
#define MOD_REQUEST_SERCOS_SET_MASTERCLOCK   MOD_REQUEST(0x0108, int)
#define MOD_REQUEST_SERCOS_SET_COMMAND       MOD_REQUEST(0x010A, sercos_set_command_t)

#define MOD_REQUEST_GET_CMD_DELAY            MOD_REQUEST(0x0110, int32_t)

// -----------------------------------------------------------------------------------
// canopen specific section
// -----------------------------------------------------------------------------------

#define CANOPEN_MAXNAME 40

typedef struct canopen_object_dictionary_list {
    int       slave_id;

    int       indices_cnt;
    uint16_t *indices;
} canopen_object_dictionary_list_t;

typedef struct canopen_object_description {
    int      slave_id;               //! [in]     slave id
    uint16_t index;                  //! [in]     canopen index

    uint16_t data_type;              //! [out]    datatype
    uint8_t  object_code;            //! [out]    object code
    uint8_t  max_subindices;         //! [out]    maximum number of subindices
    char     name[CANOPEN_MAXNAME];  //! [out]    object name
} canopen_object_description_t;

typedef struct canopen_element_description {
    int      slave_id;
    uint16_t index;
    uint8_t  sub_index;

    uint8_t  value_info;            //! value infos
    uint16_t data_type;             //! element data type
    uint16_t bit_length;            //! length in bits
    uint16_t obj_access;            //! object access
    char     name[CANOPEN_MAXNAME]; //! element name
} canopen_element_description_t;

typedef struct canopen_element_value {
    int      slave_id;
    uint16_t index;
    uint8_t  sub_index;

    int      value_len;
    uint8_t *value;
} canopen_element_value_t;

#define MOD_REQUEST_CANOPEN_READ_OBJECT_DESC        MOD_REQUEST(0x0200, canopen_object_description_t)
#define MOD_REQUEST_CANOPEN_READ_ELEMENT_DESC       MOD_REQUEST(0x0201, canopen_element_description_t)
#define MOD_REQUEST_CANOPEN_OBJECT_DICTIONARY_LIST  MOD_REQUEST(0x0202, canopen_object_dictionary_list_t)
#define MOD_REQUEST_CANOPEN_READ_ELEMENT_VALUE      MOD_REQUEST(0x0203, canopen_element_value_t)
#define MOD_REQUEST_CANOPEN_WRITE_ELEMENT_VALUE     MOD_REQUEST(0x0204, canopen_element_value_t)

// -----------------------------------------------------------------------------------
// module key value interface
// -----------------------------------------------------------------------------------

typedef enum key_value_command {
    kvc_read = 0,
    kvc_write = 1,
    kvc_list = 2
} key_value_command_t;

typedef struct key_value_transfer {
    int slave_id;
    key_value_command_t command;

    uint32_t *keys; /*
                 kvc_read, kvc_write: provided by caller
                 kvc_list: provided by callee
               */
    size_t keys_len;

    char **values; /* each values[x] is a \0-terminated string
                     kvc_read, kvc_write: values is provided by caller,
                     kvc_list: values is provided by callee,
                     kvc_read, kvc_list: values[x] are provided by callee, caller has to free() values[x],
                     kvc_write: values[x] are provided by caller
                   */
    size_t values_len;

    char *error_msg; // kvc_read, kvc_write, kvc_list: provided by callee or NULL, caller has to free() error_msg
} key_value_transfer_t;

#define MOD_REQUEST_KEY_VALUE_TRANSFER              MOD_REQUEST(0x0300, key_value_transfer_t)

// -----------------------------------------------------------------------------------
// module features
// -----------------------------------------------------------------------------------

static const int MODULE_FEAT_PD         = 0x00000001;
static const int MODULE_FEAT_READ       = 0x00000004;
static const int MODULE_FEAT_WRITE      = 0x00000008;
static const int MODULE_FEAT_TRIGGER    = 0x00000010;
static const int MODULE_FEAT_CANOPEN    = 0x00001000;
static const int MODULE_FEAT_ETHERCAT   = 0x00002000;
static const int MODULE_FEAT_SERCOS     = 0x00004000;

#define MOD_REQUEST_GET_MODULE_FEAT                 MOD_REQUEST(0x2000, int)

// -----------------------------------------------------------------------------------
// module symbols
// -----------------------------------------------------------------------------------

extern "C" const char *state_to_string(module_state_t state);
extern "C" module_state_t string_to_state(const char *state);

//! cyclic process data read
/*!
  \param hdl module handle
  \param buf process data buffer
  \param bufsize size of process data buffer
  \return size of read bytes
 */
typedef ssize_t (*mod_read_t)(MODULE_HANDLE hdl, void* buf, size_t bufsize);

//! cyclic process data write
/*!
  \param hdl module handle
  \param buf process data buffer
  \param bufsize size of process data buffer
  \return size of written bytes
 */
typedef ssize_t (*mod_write_t)(MODULE_HANDLE hdl, void* buf, size_t bufsize);

//! configures module
/*!
  \param name module name
  \param config configure file
  \return handle on success, NULL otherwise
 */
typedef MODULE_HANDLE (*mod_configure_t)(const char* name, const char* config);

//! unconfigure module
/*!
  \param hdl module handle
  \return success or failure
 */
typedef int (*mod_unconfigure_t)(MODULE_HANDLE hdl);

//! set module state machine to defined state
/*!
  \param hdl module handle
  \param state requested state
  \return success or failure
 */
typedef int (*mod_set_state_t)(MODULE_HANDLE hdl, module_state_t state);

//! get module state machine state
/*!
  \param hdl module handle
  \return current state
 */
typedef module_state_t (*mod_get_state_t)(MODULE_HANDLE hdl);

//! send a request to module
/*!
  \param hdl module handle
  \param reqcode request code
  \param ptr pointer to request structure
  \return success or failure
 */
typedef int (*mod_request_t)(MODULE_HANDLE hdl, int reqcode, void* ptr);

//! module trigger callback
/*!
 * \param hdl module handle
 */
typedef void (*mod_trigger_t)(MODULE_HANDLE hdl);

#endif // __MODULE_INTF_H__

