# 🚗 CyberCAN Defense  
### Phát hiện và Phòng thủ Xâm nhập Mạng Nội Bộ Ô Tô

---

## 📌 Mục tiêu dự án

CyberCAN Defense là một hệ thống mô phỏng và kiểm chứng các hình thức **tấn công mạng trên CAN bus** trong ô tô, đồng thời đề xuất các giải pháp **phòng thủ hiệu quả** nhằm đảm bảo an toàn cho hệ thống điều khiển nội bộ xe hơi.

---

## ⚙️ Thành phần chính

### 1. Giao tiếp CAN cơ bản
- Thiết lập 2 node truyền/nhận dữ liệu qua giao thức CAN.
- Chưa tích hợp cơ chế xác thực hay mã hóa.
  
📷 *Minh họa: Giao diện truyền nhận CAN chưa có bảo vệ*

<p align="center">
  <img src="images/TRXCAN.png" alt="CAN Interface" width="600"/>
</p>

---

### 2. Giao tiếp 2 chiều giữa các node
- Hai chiều tương tác hoàn toàn qua CAN: node A gửi → node B nhận → phản hồi lại node A.
  
🔗 [\_Xem video minh họa\_](https://drive.google.com/file/d/1IhzmS7N2qZ4zpnPbRHyV2St-RuTUCvO9/view?usp=sharing)

---

### 3. Tấn công CAN: Replay & Spam Attack

- Sử dụng giao diện web trực quan để khởi tạo tấn công spam gói CAN.
- Tạo gói dữ liệu giả lặp lại nhằm làm nhiễu hệ thống đích (replay).
  
📷 *Minh họa: Giao diện tấn công CAN*

<p align="center">
  <img src="images/CAN_Attack_Interface.png" alt="CAN Attack UI" width="600"/>
</p>

🔗 [\_Video minh họa tấn công\_](https://drive.google.com/file/d/1KyYhAsOvQhmhLoRIDh1QBbgJA2HXZJNm/view?usp=sharing)

---

### 4. Phòng thủ: Phát hiện hành vi bất thường

- Sử dụng thuật toán phát hiện bất thường (ví dụ dựa trên ID, tần suất gói, checksum...).
- Gửi cảnh báo qua UART khi phát hiện hành vi lặp bất thường.

🔗 [\_Video mô phỏng cơ chế phòng thủ\_](https://drive.google.com/file/d/1eXdn-9znkbpUbPmOYnzGprltFy_cdC5y/view?usp=sharing)

---

## 🧰 Công nghệ sử dụng

- ⚙️ **CAN Protocol**
- 👨‍💻 **Arduino Uno R3**
- 📟 **MCP2551 CAN Transceiver**
- 🌐 **WebSerial (HTML + JS)** cho giao diện điều khiển tấn công
- 📡 **UART Log** để giám sát

---

## 🧠 Tính năng nổi bật

- ✅ Giao tiếp CAN song công mô phỏng thực tế.
- 🛑 Mô phỏng các kỹ thuật tấn công phổ biến: Replay, Spam.
- 🛡️ Phát hiện và cảnh báo tấn công bất thường dựa trên thống kê.
- 🔎 Giao diện web giúp điều khiển & quan sát dễ dàng.

---

## 📁 Cấu trúc thư mục

CyberCAN-Defense/
│
├── images/ # Chứa ảnh minh họa README
├── firmware/ # Code Arduino
├── web_interface/ # Web GUI để tấn công CAN
├── defense_module/ # Mã phát hiện và cảnh báo
└── README.md # Tài liệu mô tả dự án


---

## 👩‍💻 Nhóm thực hiện

- **Sinh viên:** [Tên bạn]
- **Trường:** Đại học [Tên trường]
- **Khoa:** Điện - Điện tử | Chuyên ngành: Hệ thống nhúng / An ninh mạng

---

## 📬 Liên hệ

📧 Email: yourname@uet.vnu.edu.vn  
🔗 GitHub: [github.com/yourprofile](https://github.com/yourprofile)

---


