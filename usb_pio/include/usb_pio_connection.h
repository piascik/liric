/* usb_pio_connection.h */

#ifndef USB_PIO_CONNECTION_H
#define USB_PIO_CONNECTION_H

extern int USB_PIO_Connection_Open(const char* device_name);
extern int USB_PIO_Connection_Close(void);
extern int USB_PIO_Connection_Get_Error_Number(void);
extern void USB_PIO_Connection_Error(void);
extern void USB_PIO_Connection_Error_String(char *error_string);

#endif
