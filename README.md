ESP-IDF: esp-idf-v5.4.3

## HTTPS OTA

The firmware uses the `ota_0` and `ota_1` partitions in `partitions.csv`.
After the device is connected to Wi-Fi, trigger a remote update through its
existing MQTT RPC topic with one of these payloads:

```json
{"method":"ota","params":{"url":"https://updates.example.com/zenFWCore.bin"}}
```

```json
{"method":"ota","params":"https://updates.example.com/zenFWCore.bin"}
```

The URL must use HTTPS and serve the application binary produced at
`build/zenFWCore.bin`. The server certificate must chain to a public CA in
the ESP-IDF certificate bundle. The device writes to the inactive OTA slot,
validates the image, then reboots; a failed download leaves the running
firmware unchanged.

## BLE OTA

BLE OTA advertises as `Fan-OTA`. The device starts pairing immediately after a
connection; enter the six-digit passkey printed in the serial log. OTA GATT
characteristics require an authenticated, bonded connection.

Service UUID: `0100819d-6f2c-4d9b-7544-624f00204b9a`

- Control `0100819d-6f2c-4d9b-7544-624f01204b9a` (write): `01` + firmware
  size as a 32-bit little-endian value to start, `02` to validate and reboot,
  or `03` to abort.
- Data `0100819d-6f2c-4d9b-7544-624f02204b9a` (write without response): raw
  bytes of `build/zenFWCore.bin`, sent consecutively.
- Status `0100819d-6f2c-4d9b-7544-624f03204b9a` (read/notify): one state byte,
  followed by received and expected byte counts (both 32-bit little-endian).

`sdkconfig.defaults` enables NimBLE. Apply it with `idf.py reconfigure` (or
enable Bluetooth > NimBLE in menuconfig) before building.

1. Kiểm tra version hiện tại
Vào thư mục ESP-IDF:
> cd C:\Espressif\frameworks\esp-idf
> git branch

Xem version:
> git describe --tags

2. Fetch toàn bộ tag
> git fetch --all --tags

3. Checkout sang v5.4.3
> git checkout v5.4.3
> git submodule update --init --recursive

Nếu muốn tạo branch riêng:
> git checkout -b release/v5.4.3 v5.4.3

4. Cài tool đúng cho version 5.4.3
Chạy script install:
> install.bat
hoặc:
> python tools\idf_tools.py install

5. Set lại environment
> python3 tools\idf_tools.py install-python-env
> export.bat

Sau đó kiểm tra:
> idf.py --version

Nếu đúng sẽ ra:
> ESP-IDF v5.4.3
