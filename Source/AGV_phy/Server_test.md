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

- Them nut `Tester`.
- Disable nut khi AGV dang `inprogess` hoac mat ket noi.
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

Neu AGV khong ve `idle` sau thoi gian du kien, server hien loi timeout va giu trang thai canh bao.

## Thay doi firmware

- `src/main.cpp`: dong 135, 139-140, 830, 977-981, 1006-1009.
- `src/main.cpp`: module tester dong 1102-1226.
- `src/service_agv.cpp`: dong 55, 194-200.
