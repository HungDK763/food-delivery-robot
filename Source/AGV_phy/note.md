
đây là code của AGV hiện tại tôi chưa triển khai nó truy cập vào wed server trên controler, tôi cần bạn triển khai phần wifi trong này (chỉ làm phần đó còn khi AGV nhận được gói tin rồi nó sẽ làm gì thì tôi sẽ tự code và tạo các API sự kiện return một cách rõ ràng ) 

định dạng cụ thể: AGV chỉ nhận như sau: Khay nào bàn nào -> tôi sẽ sử dụng struct này để điều khiển. 
cụ thể define theo struct: 

// đơn vị gói tin nhỏ:
stypedef struct{
    uint8_t Khay_nao;   // chỉ có thể là khay 1 hoặc khay 2 trên AGV. 
    uint8_t ban_nao;    // bàn từ 1 -> 10. 
}mess_to_AGV_t; 

typedef struct{
    mes_to_AGV_t mes[2]; // cho 2 slot trên phần cứng AGV thể hiện rõ khay bên nào dành cho bàn nào. 
    uint8_t status_home; khi nhà bếp nhấn về home, server cần gửi đến AGV trạng thái về home và nó sẽ về home. 

}AGV_podcard_t;  



{
- B0: chuẩn bị ở trạng thái về home, chuyển sang INprogess (send_rep) , Open_1(), Open_2() mở các cửa của slot. 
- B1:  kiểm tra 2 vị trí :KHAY_1 KHAY_2 đang có hàng hay không (hai macro này đại diện cho giá trị công tắc hành trình tại slot 1/2, vD KHAY_1 == 1 -> đồ ăn đc đặt vào slot 1)(nếu khay nào trống thì không cần kiểm tra)
- B2: Thỏa mãn B1 thì đợi 3000ms ( sử dụng millis() không dùng delay cho bất kỳ trường hợp nào) đóng cửa 1 và cửa 2  lại bằng hàm Closse_1(), Close_2() đóng xong thì chuyển bước 3. 
- B3:  di chuyển tới bàn được yêu cầu gần nhất ví dụ đến B3 trước đến và dừng lại chuyển sang bước 4. 
- B4:  Bàn lẻ thì cân QUAY_TRAI(), bàn chẵn thì cần QUAY_PHAI() sau khi quay xong chuyển sang bước 5. 
- B5:  bật loa gọi Speaker_On(), và mở cửa của slot tương ứng với bàn đó ví dụ trong trường hợp này là mở cửa của slot 1, (nếu cả 2 slot đều thuộc bàn này thì mở cả 2 cửa Open_1()& Open_2()). 
- B6:  tắt loa dọi Speaker_Off(), và chờ khách hàng lấy đồ ăn ra ( phát hiện bằng cách kiểm tra KHAY_1 / KHAY_2 tương ứng với slot hiện tại, nếu KHAY_ hiện tại == 0 khách đã lấy đồ ra. trong trường hợp khác không lấy ra thì sẽ time out 30s chuyển tiếp sang bước 7), sau khi khác lấy đồ ra chuyển sang bước 7 -> trước khi sang bước 7 đợi 5s( dùng millis() để kiểm tra sau 5s đóng các cửa lại rồi mới chuyển sang bước 7) ( trong trường hợp cả 2 slot thuộc 1 bàn duy nhất thì chuyển luôn sang bước 11 ). 
- B7-> B10: sẽ là tương tự từ B3 -> B6 cho bàn tiếp theo, nếu cả 2slot thuộc 1 bàn thì logic
- B11: sau khi khách lấy xong hết,thì AGV xác định done task và gửi send_rep( ... ) done task cho Server (1 lần ) và trên đường về home. 
- B12: sau khi về home, chuyển lại trạng thái send_rep(... )IDLE. sẵn sàng chờ phiên mới. 

trong bất kỳ bước nào nếu nhận được home khẩn cũng sẽ đặt target = Home_possition  và chạy về. 
trong bất kỳ trường hợp nào nhận được ESTOP -> dừng khẩn cấp robot đứng tại chỗ ( nhưng giữ lại các trạng thái của nó hiện tại, nếu sau 30s không có phản ứng gì từ server tới thì cần chạy tiếp tiến trình)
}

đây là các bước cần thực hiện, hãy triển khai trong main.cpp một cách cụ thể, chính xác, không dài dòng đi thẳng vào vấn đề. 