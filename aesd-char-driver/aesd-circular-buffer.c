/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    // Determine the total number of bytes written to the buffer
    size_t total_bytes_written = 0;
    for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        total_bytes_written += buffer->entry[i].size;
    }

    // If the requested offset is greater than the total bytes written, return NULL
    if (char_offset >= total_bytes_written) {
        return NULL;
    }

    // Handle edge case where buffer is full and in_offs == out_offs
    int is_wrapped = 0;

    // Iterate through the buffer entries until the corresponding entry for the requested offset is found
    int bytes_searched = 0;
    for (int i = buffer->out_offs; i != buffer->in_offs || !is_wrapped; i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
        if (char_offset < bytes_searched + buffer->entry[i].size) {
            *entry_offset_byte_rtn = char_offset - bytes_searched;
            return &buffer->entry[i];
        }
        bytes_searched += buffer->entry[i].size;

        if (i == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1) {
            is_wrapped = 1;
        }
    }

    // If this point is reached, the requested offset was not found in the buffer
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    // Copy the entry into the buffer at the current in_offs location
    buffer->entry[buffer->in_offs] = *add_entry;

    // Advance the in_offs location to the next position in the buffer
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // If the buffer was already full, advance the out_offs location to the next position in the buffer
    if (buffer->full) {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // If the buffer is now full, set the full flag
    buffer->full = (buffer->in_offs == buffer->out_offs);
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
