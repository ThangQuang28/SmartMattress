# Điều khiển đệm thông minh

## Bản sửa điều khiển van

- Driver giữ thứ tự dữ liệu của ref/02: IC cuối trước, MSB trước, chốt sau đủ 32 bit.
- Chân theo bản mẫu đã chạy: DS = GPIO23, SHCP = GPIO18, STCP = GPIO5; 4 IC 74HC595.
- Bổ sung thời gian setup/hold 1 micro giây và trả lỗi thực từ GPIO.
- Tắt tất cả sẽ tắt chế độ tự động để vòng điều khiển không bật van trở lại.
- Vẫn cập nhật trạng thái và kiểm tra ngắt van thủ công khi khởi tạo cảm biến thất bại.
- Kiểm tra giới hạn lệnh van, mức áp và chỉ số khoang trước khi ép kiểu.
- USB ping chỉ gửi trạng thái, không tạo phản hồi lẫn với lệnh người dùng.
- Giao diện có đặt áp riêng từng khoang và khóa bật van thủ công khi tự động đang chạy.

## Build và sử dụng

Trong ESP-IDF PowerShell, chuyển đến thư mục ide, đặt `$env:PYTHONUTF8='1'`, chạy `idf.py build`.
Nạp bằng `idf.py -p COMx flash` với đúng cổng của board.

WiFi: kết nối SmartMattress-xxxx, mật khẩu 12345678, mở http://192.168.4.1/.
USB: chạy smart_mattress_chart/Mo_web_qua_USB.bat hoặc mở index.html bằng Chrome/Edge và chọn Kết nối USB. Đóng monitor trước khi dùng USB.

## Kiểm tra trên board

1. Xác nhận dây DS/SHCP/STCP đúng 23/18/5, nguồn, mass chung, OE ở mức thấp và MR ở mức cao.
2. Sau khởi động, kiểm tra tự động tắt và 32 đầu ra tắt. Bật/tắt riêng V1, V8, V9, V16, V17, V24, V25, V32 để xác nhận thứ tự 4 IC.
3. Chạy cùng thao tác qua WiFi và USB, kiểm tra thông báo lệnh và trạng thái.
4. Khi thủ công, ngắt mọi giao diện và xác nhận đầu ra tắt sau khoảng 30 giây (cộng thời gian vòng quét).
5. Kiểm tra đủ 13 cảm biến, cân 0 khi không tải; thay chu kỳ rồi khởi động lại để kiểm tra lưu cấu hình.
6. Đặt áp riêng khoang và mức chung; bật tự động, kiểm tra bơm/xả đúng khoang. Bấm Tắt tất cả và xác nhận không tự mở lại.

Ánh xạ tự động đã được người dùng xác nhận: theo bảng tay của người dùng (P01 9-27, P02 10-28, P03 11-32, P04 12-31, P05 13-30, P06 14-29, P07 15-20, P08 16-19, P09 2-18, P10 1-17, P11 4-22, P12 5-23, P13 6-24; nạp-xả), V3 bơm. Ref/02 chỉ chứng minh driver 74HC595, không xác định đường khí. Kiểm tra đáp ứng thực tế từng đầu ra trước khi thử tự động. Các đầu ra đang dùng mức HIGH để bật theo ví dụ ref/02.

Các module MQTT/Firebase/OTA không được bật thêm trong bản sửa này. Phân vùng Vai–Eo–Hông vẫn là minh họa RD; chưa có quy tắc chính thức để chuyển thành điều khiển thực.

## Kết quả kiểm tra bản sửa

- JavaScript của index.html: đạt kiểm tra cú pháp `node --check`.
- `git diff --check`: đạt (có cảnh báo chuyển LF/CRLF của kho mã).
- Build ESP-IDF: chưa hoàn tất. Đã chạy được toolchain ngoài sandbox với Python UTF-8; bước CMake bị chặn khi component manager thay thế thư viện LVGL trong GoogleDrive: `PermissionError [WinError 5] ... managed_components/lvgl__lvgl/.devcontainer`.
- Chưa nạp firmware hoặc kích bất kỳ đầu ra phần cứng nào. File .bin cũ trong build không đại diện cho bản sửa này.

Cập nhật sau xác nhận người dùng: theo bảng tay của người dùng (P01 9-27, P02 10-28, P03 11-32, P04 12-31, P05 13-30, P06 14-29, P07 15-20, P08 16-19, P09 2-18, P10 1-17, P11 4-22, P12 5-23, P13 6-24; nạp-xả), V3 bơm đã được xác nhận trong pctl.h và log khởi động. Đã phục hồi LVGL từ cache; bản trước giữ tại ref/lvgl_build_backup. CMake đã cấu hình được, nhưng build tiếp tục lỗi ở bước sinh linker script: compiler không tìm thấy sdkconfig.h qua đường dẫn build hiện tại. Chưa tạo được firmware mới; cần xử lý đường dẫn/toolchain và kiểm tra lại trước khi nạp.

Build thành công từ bản sao đường dẫn không dấu: C:\Users\User\.codex\visualizations\2026\10\22\01a16303-7ed6-7a61-b8f7-bea83431fd15\IOT_FWCore_ascii\ide. Firmware build/zenFWCore.bin: 0x125e60 byte, còn 39% phân vùng ứng dụng. Chưa nạp board. Trong ESP-IDF PowerShell tại thư mục ide của bản sao, chạy idf.py -p COMx flash với đúng cổng thiết bị. Không dùng build cũ trong thư mục tiếng Việt.



Cập nhật mức kích van: board kích LOW; trạng thái logic ON được đảo thành LOW trước khi dịch dữ liệu. Khung đầu tiên khi khởi tạo là 0xFFFFFFFF để mọi đầu ra OFF ngay từ lần chốt đầu. Giữ nguyên thứ tự V1–V32. Đã tắt thời hạn mất kết nối cho chế độ thủ công: van giữ trạng thái đến lệnh tắt hoặc khởi động lại. Giới hạn thời gian và áp suất trong chế độ tự động vẫn áp dụng. Thông tin cũ về HIGH để bật và tự tắt thủ công sau 30 giây ở trên được thay thế bởi cập nhật này.
