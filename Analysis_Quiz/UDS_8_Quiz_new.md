# UDS_8 Quiz: ReadDataByIdentifier (0x22)

---

## 📋 문제 1: ReadDataByIdentifier 역할

**문제:**
ReadDataByIdentifier (서비스 ID 0x22)는 OTA 프로세스에서 "최종 검증" 단계에서 사용됩니다. 다음 중 옳은 설명은?

```
① AliveCounter를 실행할 준비 상태 확인
② Flash 메모리 쓰기 전 VariantCode 검증
③ WriteDataByIdentifier로 저장한 VariantCode가 정말 NVM에 저장되었는지 확인
④ CommunicationControl을 하기 전 통신 상태 조회
```

**정답: ③**

**해설:**

OTA 전체 흐름:
```
1. DiagSessionControl (확대 세션)
2. SecurityAccess (인증)
3. CommunicationControl (통신 OFF)
4. (Flash 프로그래밍)
5. CommunicationControl (통신 ON)
6. RoutineControl (AliveCounter 설정)
7. (2~3초 대기)
8. WriteDataByIdentifier → VariantCode를 NVM에 저장 ✅
9. ReadDataByIdentifier → "정말 저장되었는가?" 최종 확인 ✅ ← 이 단계!
10. EcuReset (재부팅)
```

WriteDataByIdentifier (0x2E)가 저장을 했지만, 정말 Flash에 기록되었는지 확인하려면 **ReadDataByIdentifier (0x22)로 다시 읽어야** 합니다.

- ①번: AliveCounter는 RoutineControl (0x31)의 역할
- ②번: VariantCode는 쓰기 전이 아니라 쓴 **후에** 읽음
- ④번: CommunicationControl은 enable/disable용, 상태 조회 아님

---

## 📋 문제 2: 요청 메시지 포맷 분석

**문제:**
ReadDataByIdentifier (0x22) 서비스로 DID 0xF101 (VariantCode)을 읽으려고 합니다.
진단기가 보내는 **요청 메시지의 정확한 바이트 시퀀스**는?

(대소문자 구분, 공백은 구분자일 뿐)

**정답:**
```
03 22 F1 01
```

**상세 설명:**

| Byte | 값 | 설명 |
|------|---|---|
| 0 | `0x03` | [Frame Type (0) : Length (3)] = 0x03 |
| 1 | `0x22` | Service ID (ReadDataByIdentifier) |
| 2 | `0xF1` | **DID Upper** (Big-Endian, 0xF101의 상위) |
| 3 | `0x01` | **DID Lower** (Big-Endian, 0xF101의 하위) |

**핵심:**
- **길이: 3바이트 고정** (WriteDataByIdentifier는 7바이트)
  - SID(1) + DID(2) = 3바이트
  - 데이터는 요청에 없음!
- **Big-Endian**: 0xF101 = [0xF1, 0x01]
- **포맷**: [FrameType|Length] + [SID] + [DID High] + [DID Low]

**비교:**
```
Read (0x22):   | 03 | 22 | F1 | 01 |
Write (0x2E):  | 07 | 2E | F1 | 01 | D1 | D2 | D3 | D4 |
               ↑                      ↑
         3바이트만요!            데이터 4바이트
```

---

## 📋 문제 3: 응답 메시지 구조

**문제:**
ReadDataByIdentifier (0x22) 서비스에 대한 **성공 응답**이 다음과 같습니다:
```
07 62 F1 01 12 34 56 78
```

다음 중 옳은 설명은?

```
① Byte [2:3] = "F1 01"은 요청한 DID를 다시 보내는 Echo (확인용)
② Byte [4:7] = "12 34 56 78"은 읽어온 VariantCode 데이터
③ Byte [1] = 0x62는 SID 0x22 + 0x40 (응답 SID)
④ 모두 맞음
```

**정답: ④ 모두 맞음**

**상세 분석:**

```c
응답 메시지 구조:
[0] = 07          → [Frame Type (0) | Length (7)]
[1] = 62          → SID_ReadDataByIdentifier (0x22) + 0x40 = 0x62
                    (UDS 표준: 응답 SID = 요청 SID + 0x40)
[2] = F1          → DID Upper Echo (그게 뭔 DID냐고 우리가 부른 거, 다시 보내줄게)
[3] = 01          → DID Lower Echo
[4] = 12          → Data[0] (VariantCode byte 0)
[5] = 34          → Data[1] (VariantCode byte 1)
[6] = 56          → Data[2] (VariantCode byte 2)
[7] = 78          → Data[3] (VariantCode byte 3)
```

**중요한 차이점:**

| Write (0x2E) 응답 | Read (0x22) 응답 |
|---|---|
| `03 6E F1 01` | `07 62 F1 01 12 34 56 78` |
| 길이 3바이트 | **길이 7바이트** |
| DID Echo만 | **DID Echo + 데이터** |
| 데이터 없음 | **실제 NVM 데이터 포함** |

**바이너리 확인:**
```
0xF1 (Binary: 11110001)
  → 1111 = 0xF (상위 니블)
  → 0001 = 0x1 (하위 니블)
0x01 (Binary: 00000001)
  → 0000 = 0x0 (상위 니블)
  → 0001 = 0x1 (하위 니블)
```

---

## 📋 문제 4: Write vs Read 비교

**문제:**
WriteDataByIdentifier (0x2E)와 ReadDataByIdentifier (0x22)의 **요청/응답 길이**를 비교하면?

```
write_request  : ? 바이트
write_response : ? 바이트
read_request   : ? 바이트
read_response  : ? 바이트
```

**정답:**
```
write_request  : 7 바이트  (SID + DID(2) + Data(4))
write_response : 3 바이트  (SID + DID(2) Echo)
read_request   : 3 바이트  (SID + DID(2))
read_response  : 7 바이트  (SID + DID(2) Echo + Data(4))
```

**패턴 분석:**

```
WriteDataByIdentifier (0x2E)
    요청: [SID | DID | Data]  ← 데이터를 보냄
    응답: [SID | DID]         ← 확인만

ReadDataByIdentifier (0x22)
    요청: [SID | DID]         ← 어떤 데이터 달라고 요청
    응답: [SID | DID | Data]  ← 데이터를 받음
```

**수식:**
```
데이터 길이 (4바이트) = 7 - 3 (양쪽의 합은 10)

Write:  Req (7) + Resp (3) = 10
Read:   Req (3) + Resp (7) = 10
        ↑                ↑
    서로 역구조! (대칭)
```

**검증:**
- Write: 명령만 길고, 답변은 짧음 (쓰고 "OK!" 받음)
- Read: 명령은 짧고, 답변이 길음 (요청하고 데이터 받음)

---

## 📋 문제 5: 함수 코드 트레이싱

**문제:**
다음 코드를 실행했을 때, **응답 메시지에 들어가는 데이터는?**

```c
// NVM에 저장된 값 (hexview에서 확인함)
NVM[F101] = { 0xAA, 0xBB, 0xCC, 0xDD }

// 진단기에서 요청:
// 03 22 F1 01

// readDataByIdentifier() 함수 실행
uint8 variantCode[4] = {0, 0, 0, 0};
Rte_Call_BswDcm_rEcuAbsNvm_readNvmData(DATA_ID_VariantCode, variantCode);
// ↓ variantCode 배열에 NVM 값이 복사됨

// 응답 메시지 구성:
stBswDcmData.diagDataLengthTx = 7;
stBswDcmData.diagDataBufferTx[0] = 0x07;       // Length
stBswDcmData.diagDataBufferTx[1] = 0x62;       // SID + 0x40
stBswDcmData.diagDataBufferTx[2] = 0xF1;       // DID High
stBswDcmData.diagDataBufferTx[3] = 0x01;       // DID Low
stBswDcmData.diagDataBufferTx[4] = variantCode[0];
stBswDcmData.diagDataBufferTx[5] = variantCode[1];
stBswDcmData.diagDataBufferTx[6] = variantCode[2];
stBswDcmData.diagDataBufferTx[7] = variantCode[3];
```

**응답 메시지 (Hex로 표시):**
```
?  ?  ?  ?  ?  ?  ?  ?
```

**정답:**
```
07 62 F1 01 AA BB CC DD
```

**단계별 분석:**

| 인덱스 | 값 | 출처/이유 |
|---|---|---|
| [0] | 0x07 | FrameType(0) \| Length(7) = 0x07 |
| [1] | 0x62 | SID_ReadDataByIdentifier(0x22) + 0x40 = 0x62 |
| [2] | 0xF1 | DID High Byte (0xF101의 상위) |
| [3] | 0x01 | DID Low Byte (0xF101의 하위) |
| [4] | 0xAA | NVM에서 읽은 VariantCode[0] |
| [5] | 0xBB | NVM에서 읽은 VariantCode[1] |
| [6] | 0xCC | NVM에서 읽은 VariantCode[2] |
| [7] | 0xDD | NVM에서 읽은 VariantCode[3] |

**NVM 읽기 과정:**
```
진단기 요청: 03 22 F1 01
    ↓
ECU 수신 → readDataByIdentifier() 실행
    ↓
Rte_Call_BswDcm_rEcuAbsNvm_readNvmData(DATA_ID_VariantCode, variantCode)
    ↓ Flash 메모리에서 읽음
variantCode = {0xAA, 0xBB, 0xCC, 0xDD}
    ↓
응답 조립: 07 62 F1 01 + 4바이트 데이터
    ↓
진단기 응답: 07 62 F1 01 AA BB CC DD
```

**검증 (OTA 관점):**
```
OTA 중 저장:
setVariantCode(0xAABBCCDD)
    ↓
WriteDataByIdentifier (0x2E): 07 2E F1 01 AA BB CC DD
    ↓
완료 응답: 03 6E F1 01 ✅

OTA 후 확인:
ReadDataByIdentifier (0x22): 03 22 F1 01
    ↓
응답: 07 62 F1 01 AA BB CC DD
         ↑ 
    저장한 값이 있는가? 0xAABBCCDD = YES! ✅
```

---

## 🎯 요점 정리

| 항목 | 내용 |
|------|------|
| **SID** | 0x22 |
| **역할** | NVM에서 데이터 읽기 (OTA 완료 검증) |
| **요청** | 3바이트: SID + DID(2) |
| **응답** | 7바이트: SID + DID(2) + Data(4) |
| **검증** | 길이만 (3바이트) |
| **권한** | 제한 없음 (누구나 가능) |
| **NVM 함수** | readNvmData(DATA_ID_VariantCode, ...) |
| **Big-Endian** | 0xF101 = [0xF1, 0x01] |
| **성공 응답** | 0x62 (SID + 0x40) |
| **오류** | NRC_RequestOutRange (지원 안 하는 DID) |

---

## 🔑 마지막 단계 (OTA 흐름)

```
WriteDataByIdentifier ⇄ ReadDataByIdentifier
                       ↑
                    양쪽이 같은 DID를 다룰 때
                    Write으로 저장, Read로 검증!
```

✅ **모든 UDS_8 서비스 완성!**
