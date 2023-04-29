#ifndef HID_H_
#define HID_H_

/* Global Variables */
extern volatile bool UploadStarted;
extern volatile bool UploadFinished;

/* Function Prototypes */
void USB_Reset(void);
void USB_EPHandler(uint16_t Status);

#endif /* HID_H_ */
