# AiP74HC595: tóm tắt datasheet và đối chiếu với code đệm

Nguồn: `spec/AiP74HC595-DATASHEET-1770684383nhSgh0CvUE.pdf` (Wuxi I-CORE, bản 2023-04-B1).

## IC làm gì

Thanh ghi dịch 8 bit: nạp nối tiếp, xuất song song, có thanh ghi chốt và ngõ ra 3 trạng thái.
- Dữ liệu vào ở DS, dịch ở **sườn lên SHCP**.
- Dữ liệu chuyển sang thanh ghi chốt ở **sườn lên STCP**.
- Ngõ ra Q0–Q7 chỉ có điện khi **/OE = thấp**. /OE cao thì ngõ ra thả nổi (Z).
- **/MR = thấp** xóa thanh ghi dịch, không xóa thanh ghi chốt.
- Q7S (chân 9) nối sang DS của IC kế tiếp để nối nhiều IC.

## Sơ đồ chân (DIP16 / SOP16 / TSSOP16)

| Chân | Tên | Chức năng |
|---|---|---|
| 15, 1–7 | Q0, Q1–Q7 | Ngõ ra song song |
| 8 | GND | Mass |
| 9 | Q7S | Ngõ ra nối tiếp (sang IC kế) |
| 10 | /MR | Reset, tích cực thấp |
| 11 | SHCP | Xung dịch |
| 12 | STCP | Xung chốt |
| 13 | /OE | Cho phép ngõ ra, tích cực thấp |
| 14 | DS | Dữ liệu vào |
| 16 | VCC | Nguồn |

## Bảng chân lý (rút gọn)

| SHCP | STCP | /OE | /MR | Kết quả |
|---|---|---|---|---|
| X | X | L | L | Xóa thanh ghi dịch, ngõ ra giữ nguyên |
| X | X | H | L | Xóa thanh ghi dịch, ngõ ra thả nổi (Z) |
| ↑ | X | L | H | Dịch 1 bit từ DS vào |
| X | ↑ | L | H | Chép thanh ghi dịch ra Q0–Q7 |

## Thông số điện

| Thông số | Giá trị |
|---|---|
| VCC hoạt động | 2,0 – 6,0 V (đệm dùng 3,3 V) |
| VCC tuyệt đối tối đa | 7,0 V |
| Mức cao vào VIH | 1,5 V @2 V; 3,38 V @4,5 V → khoảng 2,3 V @3,3 V |
| Mức thấp vào VIL | 0,5 V @2 V; 1,13 V @4,5 V → khoảng 1 V @3,3 V |
| Dòng ra mỗi chân Qn | 6 mA (VCC 4,5 V) để VOL ≤ 0,33 V; tuyệt đối tối đa ±35 mA |
| Dòng tổng VCC/GND | tối đa 70 mA |
| Công suất | tối đa 500 mW |

→ Ở 3,3 V, ESP32 (ra 3,3 V) điều khiển trực tiếp được. Ngõ ra chỉ đủ kéo transistor/MOSFET/ULN2803/rơ le, không kéo trực tiếp van 24 V được.

## Thời gian (25 °C)

| Thông số | @2,0 V | @4,5 V |
|---|---|---|
| Độ rộng xung SHCP/STCP tối thiểu | 75 ns | 15 ns |
| Setup DS → SHCP | 50 ns | 10 ns |
| Hold DS sau SHCP | 120 ns | 28 ns |
| Setup SHCP → STCP | 120 ns | 30 ns |
| Tần số tối đa | 9 MHz | 30 MHz |

→ Ở 3,3 V, giá trị nằm giữa hai cột. ESP32 bật/tắt GPIO mỗi lần khoảng 50–100 ns nên vẫn đủ nhưng sát. Chèn thêm 1 µs giữa các bước dịch bit sẽ chắc chắn (32 bit vẫn chỉ mất khoảng 0,1 ms).

## Đối chiếu với code (`zdriver/zen/src/74hc595_bsp.c`, `mid/logic_ic/74hc595/74hc595.c`)

| Mục | Code | Kết luận |
|---|---|---|
| Dịch bit | Đặt DS khi SHCP thấp, rồi kéo SHCP lên, bit 7 trước | Khớp |
| Chốt | STCP thấp → cao → thấp sau khi dịch đủ 32 bit | Khớp |
| Thứ tự IC | IC cuối gửi trước, IC đầu gửi sau (V1 = Q0 IC đầu) | Khớp |
| /OE, /MR | Code không điều khiển; mạch nối /OE = GND, /MR = 3,3 V | Khớp (đã xác nhận) |
| Mức 3,3 V | VCC 595 = 3,3 V | Khớp |
| Thời gian | Không có delay | Đủ nhưng sát, nên thêm 1 µs |
| Lúc cấp nguồn | Ngõ ra ngẫu nhiên tới khi firmware gửi toàn 0 | Nên nối /OE vào GPIO có kéo lên |
