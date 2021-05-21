/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2016 Analog Devices Inc.
 * All rights reserved.
 *
 * This source code is intended for the recipient only under the guidelines of
 * the non-disclosure agreement with Analog Devices Inc.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
 * ****************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <mail_box.h>


/*!
 * @brief:  The mailbox that contains a list of subscribers
 */
// Declare the mailbox list in this file so that it's private
typedef struct _m2m2_box_t {
  M2M2_ADDR_ENUM_t     address;          /**< The mailbox's address.*/
  uint8_t                     subscriber_count; /**< The number of subscribers to this mailbox.*/
  uint8_t                     subscriber_list_size; /**< The length of the subscriber array, in number of subscribers. */
  M2M2_ADDR_ENUM_t     *p_subscriber_list; /**< Points to an array of subscribers. */
}m2m2_box_t;


/* Private function prototypes ---------------------------------------------- */
static uint8_t is_mailbox(M2M2_ADDR_ENUM_t addr);
static m2m2_box_t *find_mailbox(M2M2_ADDR_ENUM_t addr);
static m2m2_box_t *find_uninit_mailbox(void);
static ADI_MAILBOX_STATUS_t mailbox_table_expand(void);

/* Private variables --------------------------------------------------------*/
/*!
 * @brief:  The master table of mailboxes.
 */
static struct _g_mailbox_table {
  uint8_t       table_size; /**< The size of the allocated table, in number of mailboxes.*/
  uint8_t       num_boxes;  /**< The number of mailboxes in the table. */
  m2m2_box_t *table;     /**< Pointer to the allocated table.*/
}g_mailbox_table;


/*!
  *@brief       Checks if an address belongs to an existing mailbox.
  *@param addr   The address to check
  *@return      1 if true, 0 if false.
 */
static uint8_t is_mailbox(M2M2_ADDR_ENUM_t addr) {
  if (find_mailbox(addr) != NULL) {
    return 1;
  } else {
    return 0;
  }
}


/*!
  *@brief       Get a pointer to a particular mailbox.
  *@param addr   The address of the mailbox to retrieve.
  *@return      A pointer to the m2m2_box_t structure with address addr.
 */
static m2m2_box_t *find_mailbox(M2M2_ADDR_ENUM_t addr) {
  for (uint8_t i = 0; i < g_mailbox_table.table_size; i++) {
    if (g_mailbox_table.table[i].address == addr) {
      return &(g_mailbox_table.table[i]);
    }
  }
  return NULL;
}


/*!
  *@brief       Find an uninitialized mailbox object.
  *@return      A pointer to the first uninitialized m2m2_box_t that was found.
 */
static m2m2_box_t *find_uninit_mailbox(void) {
  for (uint8_t i = 0; i < g_mailbox_table.table_size; i++) {
    if (g_mailbox_table.table[i].address == M2M2_ADDR_UNDEFINED) {
      return &(g_mailbox_table.table[i]);
    }
  }
  return NULL;
}


/*!
  *@brief       Re-allocates the mailbox table, making room for 2 more mailboxes.
  *@return      Status
 */
static ADI_MAILBOX_STATUS_t mailbox_table_expand(void) {
  m2m2_box_t *new_table = calloc((g_mailbox_table.table_size + 2),
                                    sizeof(m2m2_box_t));
  if (new_table == NULL) {
    return ADI_MAILBOX_MALLOC_FAILURE;
  }
  // Copy the existing entries into the new mailbox table
  memcpy(new_table,
        g_mailbox_table.table,
        g_mailbox_table.table_size * sizeof(m2m2_box_t));
  // Update the size of the table
  g_mailbox_table.table_size += 2;
  // Free the old table memory
  free(g_mailbox_table.table);
  // Update the table pointer to the new table
  g_mailbox_table.table = new_table;
  return ADI_MAILBOX_OK;
}


/*!
  *@brief       Re-allocates the subscriber list of a particular mailbox, making room for 2 more subscribers.
  *@param box   A pointer to the m2m2_box_t whose subscriber list will be expanded.
  *@return      Status.
 */
static ADI_MAILBOX_STATUS_t subscriber_list_expand(m2m2_box_t *box) {
  M2M2_ADDR_ENUM_t *new_list = calloc((box->subscriber_list_size + 2),
                                             sizeof(M2M2_ADDR_ENUM_t));
  if (new_list == NULL) {
    return ADI_MAILBOX_MALLOC_FAILURE;
  }
  // Copy the old sub list into the new one
  memcpy(new_list,
          box->p_subscriber_list,
          box->subscriber_list_size * sizeof(M2M2_ADDR_ENUM_t));
  // Update the size of the list
  box->subscriber_list_size += 2;
  // Free the old list
  free(box->p_subscriber_list);
  // Update the list pointer to the new list
  box->p_subscriber_list = new_list;
  return ADI_MAILBOX_OK;
}


/*!
  *@brief       Check if a particular subscriber exists in a particular mailbox.
  *@param mailbox_addr    The address of the mailbox to be checked.
  *@param sub_addr   The address of the subscriber to be checked.
  *@return      1 if sub_addr is subscribed to the mailbox with address mailbox_addr.
 */
static uint8_t subscriber_exists(M2M2_ADDR_ENUM_t mailbox_addr,
                                M2M2_ADDR_ENUM_t sub_addr) {
  m2m2_box_t *this_box = find_mailbox(mailbox_addr);
  //  Run through the list of subscribers to this box and see if sub_addr is one of them.
  for (uint8_t i = 0; i < this_box->subscriber_list_size; i++) {
    if (this_box->p_subscriber_list[i] == sub_addr) {
      return 1;
    }
  }
  return 0;
}


/*!
  *@brief       Add a subscriber to a mailbox.
  *@param mailbox_addr   The address of the mailbox to be subscribed to.
  *@param sub_addr    The address of the subscriber who is being subscribed to the mailbox.
  *@return      Status
 */
ADI_MAILBOX_STATUS_t mailbox_add_sub(
                              M2M2_ADDR_ENUM_t mailbox_addr,
                              M2M2_ADDR_ENUM_t sub_addr) {
  if (subscriber_exists(mailbox_addr, sub_addr)) {
    return ADI_MAILBOX_ERROR_ITEM_EXISTS;
  }
  m2m2_box_t *this_box = find_mailbox(mailbox_addr);
  // This check will short-circuit if the subscriber list has room for another sub
  if (this_box->subscriber_count >= this_box->subscriber_list_size &&
      subscriber_list_expand(this_box) == ADI_MAILBOX_MALLOC_FAILURE) {
        return ADI_MAILBOX_MALLOC_FAILURE;
  }

  // Look for the first unused slot in the subscriber list, and put the new subscriber there.
  for (uint8_t i = 0; i < this_box->subscriber_list_size; i++) {
    if (this_box->p_subscriber_list[i] == M2M2_ADDR_UNDEFINED) {
      this_box->p_subscriber_list[i] = sub_addr;
      this_box->subscriber_count += 1;
      return ADI_MAILBOX_OK;
    }
  }
  return ADI_MAILBOX_ERROR_LIST_FULL;
}


/*!
  *@brief       Remove a subscriber from the subscription list of a mailbox.
  *@param mailbox_addr   The address of the mailbox from whose list the subscriber will be removed.
  *@param sub_addr   The address of the subscriber to remove from the subscription list.
  *@return      Status.
 */
ADI_MAILBOX_STATUS_t mailbox_remove_sub(
                              M2M2_ADDR_ENUM_t mailbox_addr,
                              M2M2_ADDR_ENUM_t sub_addr) {
  m2m2_box_t *this_box = find_mailbox(mailbox_addr);

  if (!subscriber_exists(mailbox_addr, sub_addr)) {
    return ADI_MAILBOX_ERROR_ITEM_NOT_EXIST;
  }

  // Look through the subscriber list and clear every entry for this subscriber
  for (uint8_t i = 0; i < this_box->subscriber_list_size; i++) {
    if (this_box->p_subscriber_list[i] == sub_addr) {
      this_box->p_subscriber_list[i] = M2M2_ADDR_UNDEFINED;
      // There should only ever be a single entry, so break to speed up the loop
      break;
    }
  }
  // Decrement the number of subscribers to this mailbox
  this_box->subscriber_count--;
  return ADI_MAILBOX_OK;
}


/*!
  *@brief       Get the subscriber list of a particular mailbox.
  *@param mailbox_addr   The address of the mailbox whose subscription list will be retrieved.
  *@param num_subs   A pointer to an int which will contain the number of subscribers in the subscriber list.
  *@return      A pointer to the list of subscribers.
 */
M2M2_ADDR_ENUM_t *get_subscriber_list(
                                        M2M2_ADDR_ENUM_t mailbox_addr,
                                        uint8_t         *num_subs,
                                        uint8_t         *subs_list_size) {
  m2m2_box_t *this_box = find_mailbox(mailbox_addr);
  if (this_box == NULL) {
    return NULL;
  }
  *num_subs = this_box->subscriber_count;
  *subs_list_size = this_box->subscriber_list_size;
  return this_box->p_subscriber_list;
}


/*!
  *@brief       Create a new mailbox.
  *@param addr   The address that will be given to the new mailbox.
  *@param num_subs   The number of subscriber spaces to be allocated for the mailbox's subscriber list.
  *@return    Status.
 */
ADI_MAILBOX_STATUS_t add_mailbox(M2M2_ADDR_ENUM_t addr,
                                  uint8_t num_subs) {
  m2m2_box_t *this_box = NULL;
  if (is_mailbox(addr)) {
    return ADI_MAILBOX_ERROR_ITEM_EXISTS;
  }
  g_mailbox_table.num_boxes = g_mailbox_table.num_boxes + 1;
  // Check if we have space in the mailbox table for another mailbox
  if (g_mailbox_table.num_boxes >= g_mailbox_table.table_size) {
    // Expand the mailbox table
    if (mailbox_table_expand() == ADI_MAILBOX_MALLOC_FAILURE) {
      return ADI_MAILBOX_MALLOC_FAILURE;
    }
  }
  this_box = find_uninit_mailbox();
  if (this_box == NULL) {
    return ADI_MAILBOX_ERROR_LIST_FULL;
  }
  this_box->address = addr;
  this_box->subscriber_count = 0;
  this_box->p_subscriber_list = malloc(num_subs * sizeof(M2M2_ADDR_ENUM_t));
  if (this_box->p_subscriber_list == NULL) {
    return ADI_MAILBOX_MALLOC_FAILURE;
  }
  // Make sure the subscriber list is zeroed out so that we can properly find empty entries
  memset(this_box->p_subscriber_list, 0x00, num_subs * sizeof(M2M2_ADDR_ENUM_t));
  this_box->subscriber_list_size = num_subs;
  memset(this_box->p_subscriber_list,
        0x00,
        DEFAULT_MAX_MAILBOX_SUBSCRIBER_NUM * sizeof(M2M2_ADDR_ENUM_t));
  return ADI_MAILBOX_OK;
}


/*!
  *@brief       Remove a mailbox from the master mailbox table.
  *@param addr   The address of the mailbox to be removed.
  *@return      Status.
 */
ADI_MAILBOX_STATUS_t remove_mailbox(M2M2_ADDR_ENUM_t addr) {
  m2m2_box_t *this_box;
  this_box = find_mailbox(addr);
  if (this_box == NULL) {
    return ADI_MAILBOX_ERROR_BOX_NOT_EXIST;
  }
  this_box->address = M2M2_ADDR_UNDEFINED;
  return ADI_MAILBOX_OK;
}


/*!
  *@brief       Initialize the mailbox system. Must be called before doing any mailbox operatations.
  *@param num_mailboxes   The number of mailboxes for which space will be allocated.
  *@return      Status.
 */
ADI_MAILBOX_STATUS_t mailbox_list_init(uint8_t num_mailboxes) {
  // Make sure the list isn't already init'd
  if (g_mailbox_table.table_size != 0) {
    return ADI_MAILBOX_ERROR_ALREADY_INIT;
  }
  // Malloc the table of pointers to subscriber lists
  g_mailbox_table.table = malloc(num_mailboxes * sizeof(g_mailbox_table.table[0]));
  if (g_mailbox_table.table != NULL) {
    g_mailbox_table.table_size = num_mailboxes;
    g_mailbox_table.num_boxes = 0;//num_mailboxes;
    for (uint8_t i = 0; i < g_mailbox_table.table_size; i++) {
      // Make sure the mailbox list is zeroed out so that we can properly find empty entries
      memset(&g_mailbox_table.table[i], 0x00, sizeof(g_mailbox_table.table[i]));
    }
    return ADI_MAILBOX_OK;
  } else {
    return ADI_MAILBOX_MALLOC_FAILURE;
  }
}
