# Server_test.md

## Muc tieu

Them nut `Tester` tren Kitchen server de yeu cau AGV chay test case mot lan.

## Lenh gui xuong AGV

Server gui qua WebSocket `/agv`:

```json
{"cmd":"tester"}
```

Chi gui khi AGV dang `idle`.

## Xu ly UI

- Da them nut `Tester` canh `EMERGENCY STOP` tren Kitchen Controller.
- Nut `Tester` chi bam duoc khi WebSocket online va AGV dang `idle`.
- Disable nut khi AGV dang `inprogess`/`inprogress`, mat ket noi, hoac test dang chay.
- Khong cho bam lap lai khi test dang chay.
- Lenh `agv_estop` van phai gui duoc bat ky luc nao.

## Feedback can nhan

Khi AGV bat dau test:

```json
{"event":"fb","status":"inprogess","pos":"Home"}
```

Khi AGV hoan thanh:

```json
{"event":"fb","status":"idle","pos":"Home"}
```

## Timeout

Neu AGV khong ve `idle` sau 30 giay, Kitchen hien loi timeout va mo lai trang thai test.

## Thay doi code hien tai

- `data/kitchen.html`: them nut `Tester`, ham `sendTester()`, bien `TESTER_RUNNING`/`TESTER_TIMER`, timeout 30 giay, khoa nut theo WebSocket online va trang thai AGV `idle`.
- `data/kitchen.html`: khi nhan feedback AGV `status:"idle"` thi dung timeout va mo lai nut `Tester`.
- `src/main.cpp`: them relay `{"cmd":"tester"}` tu `/ws` sang AGV qua `/agv`, tra `{"ok":true}` cho Kitchen.
- Feedback AGV chap nhan theo format da define: `{"event":"fb","status":"inprogess","pos":"Home"}` va `{"event":"fb","status":"idle","pos":"Home"}`. Server se them `_relay_from:"agv"` khi relay ve Kitchen.
