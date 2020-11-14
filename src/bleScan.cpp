#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <queue>
#include <sys/ioctl.h>
#include <sstream>
#include <sys/types.h>
#include <iomanip>
#include <unistd.h>

#include "bleScan.h"

BLEScan::BLEScan(void)
{
  connect();
}

bool BLEScan::connect(void)
{
  isConnected = false;
  device_id = hci_get_route(NULL);
  if (device_id < 0)
  {
    std::cerr << "Error: Bluetooth device not found" << std::endl;
    return false;
  }
  device_handle = hci_open_dev(device_id);
  if (device_handle < 0)
  {
    std::cerr << "Error: Cannot open device: " << strerror(errno) << std::endl;
    return false;
  }
  int on = 1;
  if (ioctl(device_handle, FIONBIO, (char *)&on) < 0)
  {
    std::cerr << "Error: Could set device to non-blocking: " << strerror(errno) << std::endl;
    return false;
  }

  // set scan parameters
  hci_le_set_scan_enable(device_handle, 0x00, 0x01, 1000); // Disable Scanning on the device before setting scan parameters
  char LocalName[0xff] = { 0 };
  hci_read_local_name(device_handle, sizeof(LocalName), LocalName, 1000);
  // Scan Type: Active (0x01)
  // Scan Interval: 18 (11.25 msec)
  // Scan Window: 18 (11.25 msec)
  // Own Address Type: Random Device Address (0x01)
  // Scan Filter Policy: Accept all advertisements, except directed advertisements not addressed to this device (0x00)
  if (hci_le_set_scan_parameters(device_handle, 0x01, htobs(0x0012), htobs(0x0012), 0x01, 0x00, 1000) < 0)
  {
    std::cerr << "Error: Failed to set scan parameters: " << strerror(errno) << std::endl;
  } else {
    // Scan Interval : 8000 (5000 msec)
    // Scan Window: 8000 (5000 msec)
    if (hci_le_set_scan_parameters(device_handle, 0x01, htobs(0x1f40), htobs(0x1f40), 0x01, 0x00, 1000) < 0)
    {
      std::cerr << "Error: Failed to set scan parameters(Scan Interval : 8000 (5000 msec)): " << strerror(errno) << std::endl;
      return false;
    }
  }

  // Scan Enable: true (0x01)
  // Filter Duplicates: false (0x00)
  if (hci_le_set_scan_enable(device_handle, 0x01, 0x00, 1000) < 0)
  {
    std::cerr << "Error: Failed to enable scan: " << strerror(errno) << std::endl;
    return false;
  }

  // Save the current HCI filter (Host Controller Interface)
  socklen_t olen = sizeof(original_filter);
  if (0 == getsockopt(device_handle, SOL_HCI, HCI_FILTER, &original_filter, &olen))
  {
    // Create and set the new filter
    struct hci_filter new_filter;
    hci_filter_clear(&new_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);
    if (setsockopt(device_handle, SOL_HCI, HCI_FILTER, &new_filter, sizeof(new_filter)) < 0) {
      std::cerr << "Error: Could not set socket options: " << strerror(errno) << std::endl;
    } else {
      isConnected = true;
    }
  }
  return isConnected;
}

void BLEScan::disconnect(void)
{
  setsockopt(device_handle, SOL_HCI, HCI_FILTER, &original_filter, sizeof(original_filter));
  hci_le_set_scan_enable(device_handle, 0x00, 1, 1000);
  hci_close_dev(device_handle);
}

bool BLEScan::scan(BLEPacket *packet)
{
  if (!isConnected)
  {
    std::cerr << "Error: not connected." << std::endl;
    return false;
  }

  packet->packetLength = read(device_handle, packet->buf, sizeof(packet->buf));

  if (packet->packetLength > HCI_MAX_EVENT_SIZE)
  {
    std::cerr << "Error: Buffer (" << packet->packetLength << ") is greater than HCI_MAX_EVENT_SIZE (" << HCI_MAX_EVENT_SIZE << ")" << std::endl;
  }

  if (packet->packetLength > (HCI_EVENT_HDR_SIZE + 1 + LE_ADVERTISING_INFO_SIZE))
  {
    evt_le_meta_event *metaEvent = (evt_le_meta_event *)(packet->buf + (HCI_EVENT_HDR_SIZE + 1));
    if (metaEvent->subevent == EVT_LE_ADVERTISING_REPORT)
    {
      const le_advertising_info * const info = (le_advertising_info *)(metaEvent->data + 1);
      packet->addr[19] = { 0 };
      ba2str(&info->bdaddr, packet->addr);
      packet->bdaddr = info->bdaddr;
      packet->event_type=info->evt_type;
      packet->event_length=info->length;
      packet->subevent = metaEvent->subevent;

      // now we have the entire event in packet.
      // time to break it down into data packets.
      // each data packet consists of an octet of length, a data type and data of "length" octets
      int current_offset = 0;

      if (info->length > 0)
      {
        bool data_error = false;
        while (!data_error && current_offset < info->length)
        {
          // every response starts with octets for length(00) and type(01).
          size_t data_len = info->data[current_offset];
          if (current_offset + data_len + 1 > info->length)
          {
            std::cout << "data length is longer than packet length. " << int(data_len) << " + 1 > " << info->length << std::endl;
            data_error = true;
          } else {
            packet->infoPacket.length = static_cast<int>(data_len);
            packet->infoPacket.type = char(*(info->data + current_offset + 1));

            packet->infoPacket.type = *(info->data + current_offset + 1);
            for (auto index = 0; index < char(data_len)-1; index++)
            {
              packet->infoPacket.data[index] = char((info->data + current_offset + 2)[index]);
            }

            packet->infoBlocks.insert(std::make_pair(packet->infoPacket.type, packet->infoPacket));
          }
          current_offset += data_len+1;
        }
      }
      packet->rssi = char(info->data[current_offset]);
      return true;
    }

  } else if (errno == EAGAIN)
  {
    usleep(100);
  }
  else if (errno == EINTR)
  {
    std::cerr << "Error: " << strerror(errno) << " (" << errno << ")" << std::endl;
  }
  return false;
}
