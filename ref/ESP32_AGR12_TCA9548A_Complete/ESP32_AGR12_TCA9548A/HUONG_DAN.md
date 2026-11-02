# ESP32 đọc áp suất AGR12 qua TCA9548A

Bộ code dùng Arduino framework cho ESP32, thư viện Wire có sẵn. Không cần cài thư viện cảm biến bên ngoài. Có thể đọc một đến tám AGR12 I²C, mỗi cảm biến ở một nhánh riêng của TCA9548A.

## 1. Phạm vi và cấu hình mặc định

- Board mặc định: ESP32 Dev Module dùng ESP32-WROOM-32, SDA GPIO21, SCL GPIO22.
- Nếu board của anh là ESP32-C3/S3 hoặc mạch thiết kế riêng: sửa hai GPIO trong phần USER CONFIGURATION trước khi nạp. GPIO21/22 không áp dụng cho mọi ESP32. Ví dụ C3 có thể dùng GPIO4/5 nếu hai chân này được đưa ra ngoài và không bị mạch khác sử dụng.
- TCA9548A có địa chỉ 7 bit 0x70 khi A0/A1/A2 nối GND.
- AGR12 I²C dùng địa chỉ 7 bit 0x50 theo datasheet ASAIR tháng 05/2024. Các byte địa chỉ trên dây là 0xA0 khi ghi và 0xA1 khi đọc; không nhập 0xA0 vào Wire.
- Quét các kênh CH0-CH7, tự phát hiện thiết bị ACK ở 0x50. Mỗi nhánh chỉ dành cho một AGR12; ACK không xác nhận được mã chip.
- Chu kỳ lấy mẫu mặc định 1 giây, tự dò lại cảm biến mỗi 10 giây.
- Các kênh được đọc tuần tự, không đồng thời. Một lần đo chờ 85 ms; nếu lỗi sẽ thử tối đa hai lần. Chu kỳ thực tế có thể dài hơn 1 giây khi nhiều kênh lỗi.
- Áp suất xuất trực tiếp theo kPa, không cần nhập full-scale để chuyển đổi. Mã cụ thể vẫn quyết định dải đo vật lý cho phép.

## 2. Đấu dây

Tắt nguồn khi đấu dây. Bảng dưới dành cho ESP32-WROOM-32 với cấu hình mặc định.

| Từ | Đến |
|---|---|
| ESP32 GPIO21 | TCA9548A SDA phía bus chính |
| ESP32 GPIO22 | TCA9548A SCL phía bus chính |
| Nguồn 3,3 V | TCA9548A VCC |
| GND chung | ESP32 GND + TCA GND + tất cả AGR12 GND |
| TCA A0/A1/A2 | GND, đặt địa chỉ 0x70 |
| TCA /RESET | Kéo lên 3,3 V bằng 4,7-10 kΩ nếu module chưa có |
| Nguồn 5 V ổn định (4,75-5,25 V) | AGR12 VCC |
| TCA SD0/SDA0 | AGR12 thứ nhất SDA |
| TCA SC0/SCL0 | AGR12 thứ nhất SCL |
| TCA SD1/SDA1, SC1/SCL1 | SDA, SCL của AGR12 thứ hai |
| TCA các nhánh còn lại | Tương tự đến CH7 |

Chân AGR12 theo bảng của hãng: 1=VCC, 2=GND, 3=SCL, 4=SDA, 5/6=NC. Phải xác định đúng chiều nhìn theo hình điện khí trong datasheet; không tự đếm chân chỉ dựa trên chiều đầu ống. Chân NC để hở.

**Nguồn và điện trở kéo lên là điểm cần kiểm tra:**

- Bus phía ESP32: SDA/SCL kéo lên 3,3 V; không kéo hai chân ESP32 lên 5 V. Nếu module chưa có, có thể bắt đầu với điện trở 4,7 kΩ mỗi đường cho dây ngắn, rồi kiểm tra dạng sóng thực tế.
- Phía AGR12: datasheet ghi cảm biến đã có điện trở kéo lên I²C 4,7 kΩ. Cấp cảm biến 5 V; không thêm nhiều điện trở song song không cần thiết. Kiểm tra sơ đồ module thực tế để tránh các pull-up của cùng nhánh bị nối về các nguồn khác nhau.
- TCA9548A cấp 3,3 V được dùng để ghép bus chính 3,3 V với các nhánh cảm biến 5 V theo nguyên lý chuyển mức trong datasheet TI. Không nối tắt SDA/SCL của cảm biến sang phía ESP32.
- Thêm tụ 100 nF sát chân nguồn AGR12 và TCA nếu mạch chưa có tụ.
- Nếu dùng IC TCA trần thay vì module, phải bổ sung linh kiện kéo lên và tụ nguồn theo datasheet.
- Giữ dây I²C ngắn. Không đấu nối cảm biến vào nguồn 3,3 V chỉ vì MCU dùng logic 3,3 V.

Tùy chọn phục hồi bus: nối một GPIO trống đến /RESET của TCA, vẫn giữ pull-up lên 3,3 V, rồi sửa TCA_RESET_PIN. Mặc định -1 là không nối GPIO. Nếu một nhánh giữ SDA ở mức thấp, reset phần mềm qua I²C có thể không thực hiện được; /RESET phần cứng giúp cô lập nhánh. Không có chân này thì có thể cần tắt/bật nguồn để thoát bus kẹt.

## 3. Mở và nạp bằng Arduino IDE

1. Giải nén ZIP, giữ nguyên hai file ESP32_AGR12_TCA9548A.ino và Agr12Protocol.h trong cùng thư mục ESP32_AGR12_TCA9548A.
2. Cài gói board **esp32 by Espressif Systems** trong Boards Manager. URL bổ sung khi cần: https://espressif.github.io/arduino-esp32/package_esp32_index.json
3. Mở ESP32_AGR12_TCA9548A.ino, chọn đúng board và cổng COM.
4. Kiểm tra GPIO SDA/SCL, địa chỉ TCA, các kênh được dùng.
5. Nhấn Verify rồi Upload.
6. Mở Serial Monitor ở **115200 baud**. Không có cảm biến thì chương trình vẫn chạy và in thông báo chẩn đoán.

Nếu dùng VS Code/PlatformIO, tạo dự án theo đúng board thực tế với framework Arduino, chép nội dung .ino thành src/main.cpp và chép Agr12Protocol.h vào src. Đặt monitor_speed=115200.

## 4. Cấu hình nhanh

Trong đầu file .ino:

```cpp
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr uint8_t TCA_ADDRESS = 0x70;
constexpr uint8_t AGR12_ADDRESS = 0x50;
constexpr uint8_t ENABLED_CHANNELS = 0xFF;
constexpr int TCA_RESET_PIN = -1;
constexpr uint32_t SAMPLE_PERIOD_MS = 1000;
```

ENABLED_CHANNELS là mặt nạ bit: chỉ CH0=0x01; CH0 và CH1=0x03; CH0-CH3=0x0F; cả 8=0xFF. Tần số I²C đã đặt 100 kHz để phù hợp AGR12 dù TCA hỗ trợ 400 kHz. Không giảm CONVERSION_WAIT_MS xuống dưới 80 ms.

## 5. Giao thức và kết quả

Mỗi lần đo: chọn một kênh TCA bằng byte (1 << channel), kết thúc bằng STOP; gửi hai byte AC 12 đến AGR12; chờ 85 ms; đọc ba byte DATA0, DATA1, CHECK. CHECK phải bằng DATA0 XOR DATA1. Hai byte dữ liệu ghép theo MSB trước, diễn giải thành số nguyên có dấu 16 bit rồi chia 10 để ra kPa. bar = kPa/100.

Ví dụ từ datasheet: 01 23 22 => +29,1 kPa; FE DD 23 => -29,1 kPa. Code không dùng công thức 14 bit hoặc hệ số 10-90% full-scale của cảm biến khác.

Ví dụ Serial minh họa, không phải số đo phần cứng:

```text
# CH0: device ACK at 0x50
# CH1: device ACK at 0x50
# Detected mask=0x03 (ACK is not a sensor identity check)
time_ms,channel,pressure_kPa,pressure_bar,status,raw_hex
1000,0,29.1,0.2910,OK,01 23 22
1087,1,0.0,0.0000,OK,00 00 00
2000,0,,,CHECKSUM_ERROR,01 23 23
```

Chỉ dùng giá trị có status=OK. Khi lỗi, hai cột áp suất để trống; không tái sử dụng giá trị cũ. Các dòng bắt đầu bằng # là thông tin chẩn đoán, bỏ các dòng này nếu tách log thành CSV. Log qua USB Serial; chương trình chưa tự tạo tệp trên máy tính.

Không tự zero hoặc làm mượt, nên số đo vẫn giữ offset và dao động của cảm biến. Độ phân giải dữ liệu 0,1 kPa không đồng nghĩa độ chính xác ±0,1 kPa. Với bản AGR đo áp dương, giá trị âm nhỏ cần được kiểm tra offset và điều kiện đo; khả năng đọc số âm không mở rộng dải đo vật lý.

## 6. Lệnh và xử lý lỗi

Gửi một ký tự qua Serial Monitor, có hoặc không có ký tự xuống dòng:

| Lệnh | Tác dụng |
|---|---|
| s | Dò địa chỉ I²C trên từng nhánh được bật |
| r | Dò lại cảm biến ngay |
| h | Hiện cấu hình và tên cột CSV |

| Thông báo | Kiểm tra |
|---|---|
| I2C_INIT_ERROR | GPIO có tồn tại và dùng được trên board đã chọn không |
| MUX_ERROR | Nguồn TCA, địa chỉ A0-A2, SDA/SCL, /RESET, nhánh bị kẹt |
| Không có cảm biến ở 0x50 | Đúng bản I²C mã C, nguồn 5 V, đúng nhánh, đúng SDA/SCL và GND chung |
| COMMAND_ERROR | Cảm biến không ACK lệnh; xem dây, nguồn, địa chỉ, mất kết nối |
| SHORT_READ | Không nhận đủ 3 byte; kiểm tra chất lượng bus và nguồn |
| CHECKSUM_ERROR | Dữ liệu sai XOR; kiểm tra nhiễu, dây, pull-up hoặc khác phiên bản giao thức |
| ISOLATE_ERROR | Không tắt được nhánh sau đọc; kiểm tra bus và chân /RESET |

Nếu scanner thấy địa chỉ khác 0x50, đối chiếu lại tài liệu đúng phiên bản trước khi sửa AGR12_ADDRESS; không tự đổi địa chỉ rồi áp giao thức này cho một cảm biến khác.

## 7. Kiểm tra đã thực hiện và giới hạn

Đã biên dịch mã C++ bằng GCC với mô phỏng Arduino/Wire và chạy kiểm tra: các frame dương/âm theo ví dụ hãng, số 0, biên số có dấu, lỗi XOR, sai chiều dài, chọn và cô lập kênh, thời gian chờ tối thiểu 80 ms, thử lại, thiếu byte, mất kết nối, định dạng CSV thành công/thất bại.

Chưa biên dịch bằng toolchain ESP32 hoặc nạp thử board/cảm biến thực tế trong phiên này. Cần Verify trong Arduino IDE với đúng board, sau đó đối chiếu số đo với đồng hồ áp suất chuẩn. Chưa xác nhận mã board ESP32, module TCA và mã đầy đủ AGR12 của anh; các GPIO và giao thức mặc định phải khớp phần cứng thực tế.

## 8. Tài liệu gốc

- ASAIR AGR12 bản I²C, tháng 05/2024, mục 7 và 10: https://www.aosong.com/userfiles/files/media/AGR12%E5%8E%8B%E5%8A%9B%E4%BC%A0%E6%84%9F%E5%99%A8%E7%B3%BB%E5%88%97%E8%AF%B4%E6%98%8E%E4%B9%A6%EF%BC%88IIC%E6%AC%BE%EF%BC%89A0-20240527.pdf
- Texas Instruments TCA9548A Rev.H, mục 7.5 và 8.2: https://www.ti.com/lit/ds/symlink/tca9548a.pdf
- Espressif Arduino-ESP32 Wire API: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html
