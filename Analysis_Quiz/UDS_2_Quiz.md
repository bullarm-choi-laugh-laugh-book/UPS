# UDS_2 (ECUReset 0x11) 퀴즈

> UDS_2_Analysis.md를 읽으며 학습한 내용을 체크하세요.
> 먼저 **문제 1~5번**을 모두 풀고, 답은 뒤에서 확인하세요!

---

## 📝 문제 1: ECUReset이 필요한 이유

**OTA (Over The Air) 플로우에서 ECUReset(0x11)이 왜 필수인가?**

다음 중 가장 적절한 답을 고르세요:

A) 매초마다 ECU의 상태를 확인하기 위해  
B) 새로운 펌웨어가 메모리에 저장된 후, **실제로 로드되어 실행되도록** 강제하기 위해  
C) 보안상 이유로 매번 리셋해야 하니까  
D) OTA가 실패했을 때 원래대로 돌아가기 위해  

---

## 📝 문제 2: ECUReset 메시지 포맷

**다음 ECUReset CAN 메시지를 분석하세요:**

```
[TX] 7D3: 02 11 01 55 55 55 55 55
[RX] 7DB: 02 51 01 AA AA AA AA AA
```

**질문:**

**(2-1) 첫 번째 메시지에서 0x11은 무엇을 의미하나?**

**(2-2) 응답에서 0x51이 나온 이유는? (0x11에서 어떻게 변했나?)**

**(2-3) 응답의 길이가 2바이트인 이유는?** (DiagSessionControl은 6바이트였는데...)

**(2-4) 요청과 응답의 Filler Byte가 다른 이유는?** (요청: 0x55, 응답: 0xAA)

---

## 📝 문제 3: 2단계 아키텍처

**UDS_2에서 세션 제어와 다르게 "2단계 구조"가 필요한 이유를 설명하세요:**

```
1단계 (BswDcm):         2단계 (EcuAbs 메인루프):
ecuReset()              REtEcuAbs_ecuAbsMain()
  ↓                       ↓
isEcuReset = TRUE    ← getDiagStatus()
응답 전송              if(isEcuReset == TRUE)
(완료!)                 Mcu_PerformReset()
                        (실제 리셋 실행)
```

**다음 중 가장 적절한 이유를 고르세요:**

A) 코드를 더 깔끔하게 만들기 위해  
B) BswDcm에서 즉시 리셋하면 응답을 보낼 수 없고, 모든 리소스를 안전하게 정리할 수 없기 때문  
C) EDU 하드웨어에서 요구하는 표준  
D) 리셋을 두 번 해야 하니까  

---

## 📝 문제 4: 세션 제약 & 보안

**강의록 P.6의 "Table 23"을 보면, DiagSessionControl(0x10)과 ECUReset(0x11)은 양쪽 세션에서 모두 가능합니다.**

**그런데 SecurityAccess(0x27)와 WriteDataByIdentifier(0x2E)는 Extended 전용입니다.**

**질문: 왜 ECUReset은 보안 제약이 없을까?**

다음 중 가장 적절한 이유를 고르세요:

A) ECUReset은 매우 위험하니까 누구나 할 수 있게 열어둔 것  
B) ECUReset 자체는 어떤 데이터도 수정하지 않기 때문 (다만 실제 이후 처리는 다를 수 있음)  
C) 진단 표준에서 그렇게 규정했으니까  
D) Default 세션에서도 ECU 상태를 초기화할 수 있게 하기 위해  

---

## 📝 문제 5: 코드 흐름 추적

**다음 시나리오를 추적하세요.**

진단기가 ECU를 하드 리셋하라고 요청합니다:

```
입력:  isPhysical=TRUE
       diagReq[] = [0x02, 0x11, 0x01, 0x55, 0x55, 0x55, 0x55, 0x55]
             (CAN에서 받은 원본 데이터)

단계 1) REoiBswDcm_pDcmCmd_processDiagRequest() 실행
```

**질문 5-1:**
ECUReset(0x11) 요청을 받았을 때, processDiagRequest()가 diagDataBufferRx에 복사하는 데이터의 **첫 2바이트**는?

```
diagDataBufferRx[0] = ???
diagDataBufferRx[1] = ???
```

---

```
단계 2) REtBswDcm_bswDcmMain() 실행
```

**질문 5-2:**
REtBswDcm_bswDcmMain()에서 다음 코드가 실행됩니다:

```c
if(serviceId == SID_EcuReset) {
    errorResult = ecuReset();
}
```

이때 **serviceId의 값**은?

---

```
단계 3) ecuReset() 함수 실행
```

**질문 5-3:**
ecuReset() 함수가 서브함수를 확인합니다:

```c
uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu;
```

이때 **subFunctionId의 값**은?

**질문 5-4:**
ecuReset()에서 성공했을 때, 가장 중요한 동작은?

A) 즉시 CPU를 리셋한다 (Mcu_PerformReset() 호출)  
B) isEcuReset 플래그를 TRUE로 설정한다  
C) 에러 카운터를 증가시킨다  
D) 세션을 Default로 변경한다  

---

```
단계 4) ecuReset() 응답 메시지 구성
```

**질문 5-5:**
ecuReset()이 성공했을 때 생성되는 응답 메시지를 채워보세요:

```
diagDataBufferTx[0] = ???
diagDataBufferTx[1] = ???
diagDataBufferTx[2] = ???
(나머지는 Filler Byte 0xAA)
```

---

```
단계 5) 메인 루프 계속 실행
```

**질문 5-6:**
응답이 전송된 후, EcuAbs 메인 루프에서 다음이 실행됩니다:

```c
Rte_Call_EcuAbs_rDcmCmd_getDiagStatus(&stDiagStatus);
if(stDiagStatus.isEcuReset == TRUE) {
    Mcu_PerformReset();   // ← 실제 리셋
}
```

**질문: getDiagStatus()는 어디서 뭘 복사해올까?**

```c
void REoiBswDcm_pDcmCmd_getDiagStatus(P_stDiagStatus pstDiagStatus)
{
	(void)memcpy(pstDiagStatus, ???, sizeof(ST_DiagStatus));
}
```

빈칸을 채우세요.

**질문 5-7:**
최종 리셋 흐름을 정리하면:

```
[1] ecuReset()에서 isEcuReset = TRUE 설정
    → 응답 0x51 전송
    ↓
[2] 진단기가 응답 수신
    ↓
[3] EcuAbs 메인 루프
    → getDiagStatus() 호출
    → isEcuReset이 TRUE인지 확인
    → TRUE면 Mcu_PerformReset() 실행
    ↓
[4] ECU 하드웨어 리셋!!!

이 구조의 장점은?**

A) BswDcm에서 응답을 완료한 후 리셋하므로, 진단기가 응답을 제대로 받을 수 있음  
B) 리셋 전에 모든 리소스(NVM, CAN 등)를 안전하게 정리할 수 있음  
C) 계층 분리: 진단 프로토콜(BswDcm) vs 시스템 제어(EcuAbs)  
D) 모두 맞음 ✅  

---

---

---

# 🎯 답 & 해설

## 문제 1 정답: ECUReset이 필요한 이유

**정답: B) 새로운 펌웨어가 메모리에 저장된 후, 실제로 로드되어 실행되도록 강제하기 위해**

### 해설

OTA 플로우를 보면:

```
Step 1: [0x10] DiagSessionControl
        → Default → Extended 전환
        
Step 2: [0x27] SecurityAccess
        → Seed/Key 인증
        
Step 3: [0x2E] WriteDataByIdentifier
        → 새 펌웨어를 Flash/NVM에 쓰기 ✅ 저장됨!
        → 하지만 아직 실행은 안 됨
        
Step 4: [0x11] ECUReset ← ★ 필수!
        → ECU 하드웨어 리셋 강제
        → CPU가 재부팅
        ↓
Step 5: 부팅 시작
        → Bootloader 실행
        → 새 펌웨어 로드
        → 새 펌웨어 실행! 🎉
```

**ECUReset이 없으면?**
→ 새 펌웨어가 메모리에만 있고, 구 펌웨어가 계속 실행됨  
→ OTA 실패!

**참고:** 강의록 **P.25 "Uds : Ecureset"**, 이미지 `25_image_0.png`, `25_image_1.png`, `25_image_2.png`, `25_image_3.png`

---

## 문제 2 정답: ECUReset 메시지 포맷

### 2-1 첫 번째 메시지에서 0x11은?

**답: ECUReset 서비스 ID**

```
0x02 = Single Frame, 길이 2
0x11 = 서비스 ID (ECUReset)
0x01 = SubFunc = HardReset
0x55 = Filler bytes
```

**참고:** 강의록 **P.23 "Table 33 - Request message definition"**: ECUReset Request SID = 0x11

---

### 2-2 응답에서 0x51이 나온 이유

**답: `0x11 + 0x40 = 0x51`**

ISO 14229의 긍정 응답 규칙:
- 서비스 ID + 0x40 = 긍정 응답

```
요청:  0x11 (ECUReset)
응답:  0x51 (0x11 + 0x40 = 긍정 응답)
```

비교:
- DiagSessionControl: 0x10 → 0x50
- ECUReset: 0x11 → 0x51
- SecurityAccess: 0x27 → 0x67

**참고:** 강의록 **P.24 "Table 35 - Positive Response Message Definition"**: ECUReset Response SID = 0x51

---

### 2-3 응답의 길이가 2바이트인 이유

**답: DiagSessionControl은 P2/P2* 타이밍을 반환해야 하지만, ECUReset은 그럴 필요가 없기 때문**

```
DiagSessionControl 응답:
[프레임] [SID] [SubFunc] [P2_Hi] [P2_Lo] [P2*_Hi] [P2*_Lo]
  1바이트  1    1        1      1      1       1      = 총 7바이트
→ 길이 6

ECUReset 응답:
[프레임] [SID] [SubFunc]
  1바이트  1    1            = 총 3바이트
→ 길이 2
```

왜?
- DiagSessionControl은 **타이밍 정보**가 필요 (진단기가 응답 대기 시간을 알아야 함)
- ECUReset은 추가 정보가 필요 없음 (단순히 리셋하면 끝)

**참고:** 강의록 **P.24 "Table 35"**: 응답이 짧음

---

### 2-4 Filler Byte가 다른 이유

**답: CAN 프로토콜 규칙**

```
강의록 P.11:
"Request 메시지는 Filler Byte 55 hex로 채워진다"
"Response 메시지는 Filler Byte AA hex로 채워진다"
```

의미:
- **0x55 (요청)**: 진단기 → ECU (이진수: 01010101)
- **0xAA (응답)**: ECU → 진단기 (이진수: 10101010)

패턴이 다른 이유:
- 메시지 방향을 명확히 구분
- 통신 라인의 완전성 확인 (bit pattern 다양성)

**참고:** 강의록 **P.24 "Sub-function parameter"**: Filler Byte 정의

---

## 문제 3 정답: 2단계 아키텍처

**정답: B) BswDcm에서 즉시 리셋하면 응답을 보낼 수 없고, 모든 리소스를 안전하게 정리할 수 없기 때문**

### 해설

만약 BswDcm에서 즉시 리셋하면:

```
ecuReset()
{
    Mcu_PerformReset();  // ← 즉시 리셋!
    // ↓
    // [응답 메시지 생성 코드는 여기서 영원히 실행 안 됨!]
    // CAN 전송도 못 함
}
```

결과:
❌ 진단기는 응답을 못 받음 (Timeout 발생)  
❌ NVM이 저장 안 될 수 있음  
❌ CAN 버스가 불안정해질 수 있음  

**2단계 구조의 장점:**

```
단계 1: BswDcm (진단 프로토콜 처리)
  ├─ 플래그 설정: isEcuReset = TRUE
  ├─ 응답 생성 및 CAN 전송 완료!
  └─ 리셋 금지 ✅

단계 2: EcuAbs (시스템 관리)
  ├─ 모든 주변장치 정상 종료 (I/O, NVM, CAN 클로징)
  ├─ 리소스 정리 완료
  └─ 이제 안전하게 Mcu_PerformReset() 호출 ✅
```

**참고:** 강의록 **P.25 "Uds : Ecureset"**, 이미지 `25_image_3.png`: EcuAbs에서 호출하는 예시

---

## 문제 4 정답: 세션 제약 & 보안

**정답: B) ECUReset 자체는 어떤 데이터도 수정하지 않기 때문 (다만 실제 이후 처리는 다를 수 있음)**

### 해설

보안 관점에서:

| 서비스 | 위험도 | 데이터 수정? | 세션 제약 |
|--------|--------|------------|---------|
| SecurityAccess(0x27) | 🔴 높음 | 인증 정보 변경 | Extended 전용 |
| WriteDataByIdentifier(0x2E) | 🔴 높음 | NVM/Flash 수정! | Extended 전용 |
| ECUReset(0x11) | 🟡 보통 | 데이터 수정 배 | 제약 없음 |

**ECUReset이 안전한 이유:**
- 리셋 자체는 메모리의 **값**을 바꾸지 않음
- 단순히 CPU를 재부팅할 뿐
- 실제 펌웨어 변경은 이미 SecurityAccess 후에만 가능

**따라서:**
- Default 세션에서도 ECU 상태 확인 위해 리셋 가능
- 실제 중요한 보안은 SecurityAccess와 Write에 있음

**참고:** 강의록 **P.6 "Table 23"**: DiagSessionControl, ECUReset 모두 양쪽 세션

---

## 문제 5 정답: 코드 흐름 추적

입력 분석:
```
diagReq[] = [0x02, 0x11, 0x01, 0x55, 0x55, 0x55, 0x55, 0x55]
            └─0    ─1    ─2    ─3~7
              │     │     │     
            길이  SID  SubFunc Padding
```

---

### 5-1 diagDataBufferRx에 복사되는 첫 2바이트

**답: 0x11, 0x01**

```c
memcpy(diagDataBufferRx, &diagReq[1], 2);
//                       ↑ diagReq[1]부터 시작
//                       = &[0x11, 0x01, 0x55, ...]
```

따라서:
- `diagDataBufferRx[0] = 0x11` (SID)
- `diagDataBufferRx[1] = 0x01` (SubFunc = HardReset)

---

### 5-2 serviceId의 값

**답: 0x11**

```c
serviceId = stBswDcmData.diagDataBufferRx[0]
          = 0x11  ← ECUReset Service
```

---

### 5-3 subFunctionId의 값

**답: 0x01**

```c
subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu
             = 0x01 & 0x7F
             = 0x01  ← HardReset
```

---

### 5-4 ecuReset()에서의 가장 중요한 동작

**정답: B) isEcuReset 플래그를 TRUE로 설정한다**

```c
else if(subFunctionId == SFID_EcuReset_HardReset)
{
    stBswDcmData.stDiagStatus.isEcuReset = TRUE;
    // ← 핵심! 실제 리셋 실행 아님
}
```

이유:
- **즉시 리셋하면 응답을 못 보냄** (문제 3 해설 참고)
- **플래그만 설정하고**, EcuAbs에서 나중에 처리

**참고:** 강의록 **P.24 코드**, 이미지 `24_image_0.png`

---

### 5-5 응답 메시지 구성

**답:**

```c
diagDataBufferTx[0] = (0 << 4u) | 2 = 0x02
  // SINGLE(0) | 길이2

diagDataBufferTx[1] = 0x11 + 0x40 = 0x51
  // SID + 0x40 = 긍정 응답

diagDataBufferTx[2] = 0x01
  // SubFunc 에코 (HardReset)
```

최종:
```
[7DB] 02 51 01 AA AA AA AA AA
      └─┬─┘ ├─ 응답  └─────────┘
        │   │        Filler Byte
        │   Subfunction 에코
        └─ SINGLE, 길이2
```

**참고:** 강의록 **P.24 "Table 35"**: 응답 메시지 정의

---

### 5-6 getDiagStatus()의 복사 소스

**답: `&stBswDcmData.stDiagStatus`**

```c
void REoiBswDcm_pDcmCmd_getDiagStatus(P_stDiagStatus pstDiagStatus)
{
	(void)memcpy(pstDiagStatus, 
	             &stBswDcmData.stDiagStatus,  // ← 여기서 복사
	             sizeof(ST_DiagStatus));
}
```

의미:
- BswDcm에서 관리하는 **stDiagStatus 구조체**를 복사
- 여기에 **isEcuReset = TRUE**가 저장되어 있음
- EcuAbs가 이걸 읽어서 리셋 판단

**참고:** 강의록 **P.24 코드**: getDiagStatus() 구현

---

### 5-7 이 구조의 장점

**정답: D) 모두 맞음 ✅**

```
장점 1: 응답 완성
  ecuReset() → 응답 메시지 생성 & CAN 전송
  ↓ 진단기가 응답 수신 ✅

장점 2: 리소스 안전 정리
  EcuAbs에서 모든 주변장치 CloseOut
  ↓ NVM 저장, CAN 정지, etc ✅

장점 3: 계층 분리
  BswDcm = 진단 프로토콜만
  EcuAbs = 시스템 제어만
  ↓ 책임 분리, 유지보수 용이 ✅
```

**참고:** 강의록 **P.25 "Uds : Ecureset"**: 아키텍처 설명

---

## 📊 채점 기준

| 문제 | 만점 | 난이도 | 핵심 개념 |
|------|------|--------|---------|
| 1 | 1점 | ⭐ 쉬움 | OTA 플로우 |
| 2 | 4점 | ⭐⭐ 중간 | 메시지 포맷 |
| 3 | 1점 | ⭐⭐⭐ 어려움 | 2단계 아키텍처 |
| 4 | 1점 | ⭐⭐ 중간 | 보안과 세션 |
| 5 | 7점 | ⭐⭐⭐⭐ 매우 어려움 | 전체 흐름 추적 |
| **합계** | **14점** |  |  |

---

## 🎓 결과 해석

```
13-14점: 완벽! UDS_3/4로 진행해도 문제 없음 🚀
10-12점: 좋음! 좀 더 봐도 되지만, 헷갈리는 부분 재확인 추천 📖
7-9점:   보통! UDS_2_Analysis.md의 "1️⃣ 코드 비교" 섹션 한 번 더 읽기 🔄
4-6점:   아직도! P.24 코드를 천천히 추적해보기 🐢
0-3점:   다시 한 번! UDS_2_Analysis.md 처음부터 읽기 📚
```

---

## 🔗 UDS_1과 UDS_2 비교 요약

| 항목 | UDS_1 (0x10) | UDS_2 (0x11) |
|------|--------------|--------------|
| **역할** | 세션 전환 | 하드웨어 리셋 |
| **응답** | 0x50 + P2/P2* | 0x51 |
| **응답 길이** | 6바이트 | 2바이트 |
| **실행 시점** | 즉시 처리 | 플래그 설정 후 나중에 |
| **목적** | 다른 서비스 접근 제어 | 펌웨어 적용 강제 |
| **OTA에서 위치** | 첫 단계 | 마지막 단계 |

---

**점수 나왔으면 말씀해주세요! 그리고 난처했던 부분이 있으면 설명해드릴 수 있습니다.** 😊
