// Jacek Fedorynski <jfedor@jfedor.org>
// http://www.jfedor.org/

// This is meant to be run on an Arduino Leonardo (or similar) with a USB Host Shield
// and two USB mice connected using a USB hub.
//
// We keep screen position of both mice and alternate between them when sending
// a report to the host, resulting in two (rapidly blinking) mouse cursors.
// When buttons are clicked, that mouse gets exclusive ownership of the cursor
// for as long as the buttons are pressed.
//
// It works on Windows, Linux and Mac with no additional software on the computer.
//
// USB mice normally report relative movement, but we use absolute positioning when
// reporting to the host to be able to switch between the two cursors.

#include <hidboot.h>
#include <usbhub.h>
#include "HID.h"

// we use a custom HID report descriptor with absolute mouse positioning
static const uint8_t outReportDescriptor[] PROGMEM = {
  0x05, 0x01,           // USAGE_PAGE (Generic Desktop)
  0x09, 0x02,           // USAGE (Mouse)
  0xa1, 0x01,           // COLLECTION (Application)
  0x09, 0x01,           //   USAGE (Pointer)
  0xA1, 0x00,           //   COLLECTION (Physical)
  0x85, 0x01,           //     REPORT_ID (1)
  0x05, 0x09,           //     USAGE_PAGE (Button)
  0x19, 0x01,           //     USAGE_MINIMUM (1)
  0x29, 0x03,           //     USAGE_MAXIMUM (3)
  0x15, 0x00,           //     LOGICAL_MINIMUM (0)
  0x25, 0x01,           //     LOGICAL_MAXIMUM (1)
  0x95, 0x03,           //     REPORT_COUNT (3)
  0x75, 0x01,           //     REPORT_SIZE (1)
  0x81, 0x02,           //     INPUT (Data,Var,Abs)
  0x95, 0x01,           //     REPORT_COUNT (1)
  0x75, 0x05,           //     REPORT_SIZE (5)
  0x81, 0x03,           //     INPUT (Const,Var,Abs)
  0x05, 0x01,           //     USAGE_PAGE (Generic Desktop)
  0x09, 0x30,           //     USAGE (X)
  0x09, 0x31,           //     USAGE (Y)
  0x16, 0x00, 0x00,     //     LOGICAL_MINIMUM(0)
  0x26, 0xff, 0x7f,     //     LOGICAL_MAXIMUM(32767)
  0x75, 0x10,           //     REPORT_SIZE (16)
  0x95, 0x02,           //     REPORT_COUNT (2)
  0x81, 0x02,           //     INPUT (Data,Var,Abs)
  0xC0,                 //   END_COLLECTION
  0xC0,                 // END COLLECTION
};

// this is the report we get from the two connected mice
struct inMouseReport {
  int8_t buttons;
  int8_t dx;
  int8_t dy;
};

// this is the report we send to the host
struct outMouseReport {
  int8_t buttons;
  int16_t x;
  int16_t y;
};

// if this is -1 then we oscillate between the two mice
// if this is 0 or 1 then we show that mouse exclusively because it's currently pressing buttons
int8_t owner = -1;
// for keeping track of whose turn it is
int8_t nextId = 0;
outMouseReport outReport[2];

class MyReportParser : public HIDReportParser
{
  public:
    MyReportParser(int _id) : id(_id), x(16384), y(16384) {}
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
  private:
    int8_t id;
    int32_t x, y;
};

void MyReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  inMouseReport *inReport = (inMouseReport*) buf;

  x += inReport->dx * 18; // if you screen isn't 16:9 you might want to change these
  y += inReport->dy * 32; // also if you want to change sensitivity

  x = max(0, min(32767, x));
  y = max(0, min(32767, y));

  outReport[id].buttons = inReport->buttons;
  outReport[id].x = x;
  outReport[id].y = y;
}

USB Usb;
USBHub Hub(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE> HidMouse0(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE> HidMouse1(&Usb);
MyReportParser Parser0(0);
MyReportParser Parser1(1);

void setup()
{
  static HIDSubDescriptor node(outReportDescriptor, sizeof(outReportDescriptor));
  HID().AppendDescriptor(&node);

  Serial.begin(115200);

  if (Usb.Init() == -1) {
    Serial.println("OSC did not start.");
  }

  delay(200);

  HidMouse0.SetReportParser(0, &Parser0);
  HidMouse1.SetReportParser(0, &Parser1);
}

void loop()
{
  Usb.Task();

  // if a mouse is pressing buttons, give it ownership of the cursor
  for (int i = 0; i < 2; i++) {
    if ((owner == -1) && (outReport[i].buttons != 0)) {
      owner = i;
    }
  }

  int sendId = owner != -1 ? owner : nextId;
  nextId = (nextId + 1) % 2;

  HID().SendReport(1, &outReport[sendId], sizeof(outReport[sendId]));

  // if a mouse is no longer pressing buttons, go back to oscillating between the two
  for (int i = 0; i < 2; i++) {
    if ((owner == i) && (outReport[i].buttons == 0)) {
      owner = -1;
    }
  }
}
