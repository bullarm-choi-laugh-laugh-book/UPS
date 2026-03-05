6# UDS_4 (SecurityAccess 0x27 + Table) 퀴즈

> UDS_4_Analysis.md를 읽으며 학습한 내용을 체크하세요.
> 먼저 **문제 1~6번**을 모두 풀고, 답은 뒤에서 확인하세요!

---

## 📝 문제 1: static 변수의 역할

**코드에서 특이한 점:**

```c
static uint16 genKey;  // ← 함수 내부의 static 변수

static uint8 securityAccess(void)
{
	// RequestSeed 처리:
	if(UT_getTimeEcuAlive1ms() % 3 == 1)
	{
		seed = 0x1234;
		genKey = 0x4567;  // ← RequestSeed에서 설정
	}
	
	// SendKey 처리:
	else if(subFunctionId == SFID_SecurityAccess_SendKey)
	{
		key = ((uint16)stBswDcmData.diagDataBufferRx[2] << 8u) 
			| stBswDcmData.diagDataBufferRx[3];
		
		if(key == genKey)  // ← SendKey에서 검증!
		{
			stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;
		}
	}
}
```

**질문: RequestSeed에서 설정한 genKey = 0x4567이 SendKey에서 왜 여전히 0x4567일까?**

다음 중 가장 적절한 답을 고르세요:

A) genKey가 global 변수이기 때문  
B) genKey가 **static 변수**이기 때문에, 함수가 끝나도 메모리에 유지됨  
C) C 언어에서 모든 변수는 함수 호출 사이에 유지됨  
D) SendKey 함수가 RequestSeed의 메모리를 복사함  

---

## 📝 문제 2: 시간 기반 Seed/Key 선택

**코드 분석:**

```c
uint32 ecu_time_ms = UT_getTimeEcuAlive1ms();

if(ecu_time_ms % 3 == 1)
{
	seed = 0x1234;
	genKey = 0x4567;
}
else if(ecu_time_ms % 3 == 2)
{
	seed = 0x2345;
	genKey = 0x5678;
}
else  // (ecu_time_ms % 3 == 0)
{
	seed = 0x3456;
	genKey = 0x6789;
}
```

**질문 2-1:**
ECU 부팅 후 경과 시간이 다음일 때, 어떤 Seed와 Key가 선택될까?

- **경과 시간 = 1000ms** → `1000 % 3 = ???` → Seed: `???`, Key: `???`
- **경과 시간 = 2000ms** → `2000 % 3 = ???` → Seed: `???`, Key: `???` 
- **경과 시간 = 3000ms** → `3000 % 3 = ???` → Seed: `???`, Key: `???`

**질문 2-2:**
UDS_3 vs UDS_4의 차이점은?

다음 중 옳은 것을 모두 고르세요 (복수 선택):

A) UDS_3: 1가지 Seed/Key, UDS_4: 3가지 Seed/Key 테이블  
B) UDS_3: 고정값, UDS_4: 시간 기반 동적 선택  
C) UDS_4는 모듈로 연산으로 3가지 모드를 순환  
D) UDS_4는 더 높은 보안을 제공 (Replay Attack 방지)  

---

## 📝 문제 3: Seed와 Key의 관계

**강의록 P.30의 테이블:**

| Seed | Key |
|------|-----|
| 0x1234 | 0x4567 |
| 0x2345 | 0x5678 |
| 0x3456 | 0x6789 |

**CAN 로그:**

```
[시간 T1]
TX [7D3] 02 27 01 55 55 55 55 55    ← RequestSeed
RX [7DB] 04 67 01 12 34 AA AA AA    ← Seed = 0x1234
                       ↑  ↑

진단기가 테이블 확인:
0x1234 → ????? (대응 Key 찾기)

TX [7D3] 04 27 02 ?? ?? 55 55 55    ← SendKey
RX [7DB] 02 67 02 AA AA AA AA AA    ← 성공!
```

**질문:**
빈칸을 채워보세요. SendKey에서 진단기가 보내는 Key는?

A) 0x1234  
B) 0x4567 ← 정담!  
C) 0x5678  
D) 0x6789  

---

## 📝 문제 4: CAN 로그 분석

**다음 3가지 SecurityAccess 시도를 분석하세요:**

### SecurityAccess_1

```
TX [7D3] 02 27 01 55 55 55 55 55
RX [7DB] 04 67 01 12 34 AA AA AA     ← Seed = 0x1234
TX [7D3] 04 27 02 45 67 55 55 55     ← Key = 0x4567
RX [7DB] 02 67 02 AA AA AA AA AA     ← 성공! ✅
```

### SecurityAccess_2

```
TX [7D3] 02 27 01 55 55 55 55 55
RX [7DB] 04 67 01 23 45 AA AA AA     ← Seed = 0x2345
TX [7D3] 04 27 02 56 78 55 55 55     ← Key = 0x5678
RX [7DB] 02 67 02 AA AA AA AA AA     ← 성공! ✅
```

### SecurityAccess_3

```
TX [7D3] 02 27 01 55 55 55 55 55
RX [7DB] 04 67 01 34 56 AA AA AA     ← Seed = 0x3456
TX [7D3] 04 27 02 67 89 55 55 55     ← Key = 0x6789
RX [7DB] 02 67 02 AA AA AA AA AA     ← 성공! ✅
```

**질문 4-1:**
3가지 시도가 모두 성공한 이유는? (Seed와 Key의 대응 관계)

**질문 4-2:**
만약 SecurityAccess_2에서 진단기가 잘못된 Key(0x4567)를 보냈다면?

다음 중 응답은?

A) [0x02][0x67][0x02][...] - 성공  
B) [0x03][0x7F][0x27][0x35][...] - NRC_InvalidKey ← 정답!  
C) [0x03][0x7F][0x27][0x13][...] - NRC_IncorrectMessageLength  
D) 아무 응답 안 함  

---

## 📝 문제 5: 코드 흐름 추적 (시간 기반)

**시나리오:**

```
ECU 부팅 후 경과 시간 = 1001ms

SecurityAccess RequestSeed 호출
```

**질문 5-1:**
`UT_getTimeEcuAlive1ms() % 3` 계산:

```
1001 % 3 = ???
```

**질문 5-2:**
어떤 Seed와 Key가 선택될까?

```
if(UT_getTimeEcuAlive1ms() % 3 == 1)  // ← 1001 % 3 = 1인가?
{
	seed = 0x1234;
	genKey = 0x4567;
}
```

답: Seed = `???`, genKey = `???`

**질문 5-3:**
RequestSeed 응답에서 진단기에게 전송되는 Seed는?

```
diagDataBufferTx[3] = (uint8)((seed & 0xFF00u) >> 8u)
diagDataBufferTx[4] = (uint8)(seed & 0xFFu)
```

CAN 메시지로는 `[0x04][0x67][0x01][??][??]`

빈칸: High Byte = `???`, Low Byte = `???`

**질문 5-4:**
ECU 메모리에는 뭐가 저장될까?

```
ECU:   genKey = 0x4567 (static 메모리) ← SendKey에서 검증 시 사용!
진단기: Seed(0x1234) 수신 → 테이블에서 Key(0x4567) 찾기
```

이제 진단기가 SendKey에서 Key를 보냈을 때, ECU가 검증하는 조건은?

```c
if(key == genKey)  // 0x4567 == 0x4567 ✅
```

---

## 📝 문제 6: UDS_3과 UDS_4 보안 비교

**시나리오: "Replay Attack" (재현 공격)**

### UDS_3의 경우 (고정값)

```
공격자의 계획:
[1] 진단기A의 CAN 로그 스니핑:
    Seed = 0x1234, Key = 0x5678 (기록!)
    
[2] 1시간 뒤, 다시 같은 Seed/Key로 공격:
    TX [0x1234의 Seed에 대해 0x5678 Key 전송]
    RX [0x67 성공 응답] ← 어?! 인증 성공! ❌ 위험!
```

### UDS_4의 경우 (테이블 기반)

```
공격자의 계획:
[1] 진단기A의 CAN 로그 스니핑 (시간 T1):
    Seed = 0x1234, Key = 0x4567 (기록!)
    
[2] 1시간 뒤 (시간 T2), 다시 같은 Seed/Key로 공격:
    TX [0x1234의 Seed에 대해 0x4567 Key 전송]
    
    하지만 그 시간 T2에는...
    UT_getTimeEcuAlive1ms() % 3 = ??? (T1과 다름!)
    
    → Seed = 0x2345 or 0x3456 (T1의 0x1234가 아님!)
    
    → genKey = 0x5678 or 0x6789 (T1의 0x4567이 아님!)
    
    RX [0x7F, NRC_InvalidKey] ← 공격 실패! ✅ 안전!
```

**질문:**
UDS_4가 UDS_3보다 안전한 이유를 설명하세요.

다음 중 옳은 것을 모두 고르세요:

A) Seed와 Key가 시간에 따라 변함  
B) 과거에 스니핑한 (Seed, Key) 쌍을 나중에 재사용 불가능  
C) **Time-based Replay Attack Protection** 제공  
D) 3가지 모드로 순환하므로 예측 불가능  

---

---

---

# 🎯 답 & 해설

## 문제 1 정답: static 변수의 역할

**정답: B) genKey가 static 변수이기 때문에, 함수가 끝나도 메모리에 유지됨**

### 해설

**static 변수의 특징:**

```c
static uint16 genKey;
```

| 특성 | 설명 |
|------|------|
| **메모리 위치** | 데이터 세그먼트 (Stack이 아닌 정적 메모리) |
| **초기화** | 프로그램 시작 시 1회만 초기화 |
| **유지** | 함수 호출이 끝나도 값이 유지됨 |
| **범위** | 함수 내부에만 접근 가능 (캡슐화) |

**NonStatic 변수와의 비교:**

```c
// ❌ 잘못된 방식 (static 없음)
uint8 securityAccess(void)
{
	uint16 genKey;  // 로컬 변수 (Stack)
	
	// RequestSeed
	genKey = 0x4567;  // Stack에 저장
	
	// 함수가 끝나면...
	// genKey는 Stack에서 제거됨!
	
	// SendKey (다시 호출)
	// genKey는 초기화되거나 쓰레기값 ❌
}

// ✅ 올바른 방식 (static)
uint8 securityAccess(void)
{
	static uint16 genKey;  // 정적 메모리
	
	// RequestSeed
	genKey = 0x4567;  // 정적 메모리에 저장
	
	// 함수가 끝나면...
	// genKey는 여전히 0x4567 유지! ✅
	
	// SendKey (다시 호출)
	// genKey = 0x4567 접근 가능 ✅
}
```

**메모리 다이어그램:**

```
┌─────────────────────┐
│ 정적 메모리 (Data)  │
│ ┌─────────────────┐ │
│ │ genKey = 0x4567 │ │ ← RequestSeed에서 설정
│ │ (여기 유지!)    │ │
│ └─────────────────┘ │
└─────────────────────┘
          ↓
      SendKey에서
      다시 접근 가능!
```

**참고:** 강의록 **P.30 코드**: securityAccess() 함수, static genKey 선언

---

## 문제 2 정답: 시간 기반 Seed/Key 선택

### 2-1 모듈로 연산 계산

**답:**

```
1000 % 3 = 1
→ seed = 0x1234, genKey = 0x4567

2000 % 3 = 2
→ seed = 0x2345, genKey = 0x5678

3000 % 3 = 0
→ seed = 0x3456, genKey = 0x6789
```

**계산 과정:**

```
1000 ÷ 3 = 333.333...
1000 - (333 × 3) = 1000 - 999 = 1 ✓

2000 ÷ 3 = 666.666...
2000 - (666 × 3) = 2000 - 1998 = 2 ✓

3000 ÷ 3 = 1000
3000 - (1000 × 3) = 3000 - 3000 = 0 ✓
```

---

### 2-2 UDS_3 vs UDS_4 차이점

**정답: A, B, C, D 모두 맞음 ✅**

| 항목 | UDS_3 | UDS_4 |
|------|-------|-------|
| **Seed/Key 종류** | 1가지 고정 | 3가지 동적 |
| **선택 방식** | 상수값 | 시간 기반 % 3 |
| **모드 순환** | No | Yes (0, 1, 2 반복) |
| **보안** | 낮음 (Replay 취약) | 높음 (시간 기반 보호) |

**참고:** 강의록 **P.30 "SecurityAccess_1/2/3"**: 실제 테이블 기반 예시

---

## 문제 3 정답: Seed와 Key의 관계

**정답: B) 0x4567**

### 해설

**테이블 매핑:**

```
Seed:     Key:
0x1234 → 0x4567
0x2345 → 0x5678
0x3456 → 0x6789
```

**CAN 분석:**

```
[1] RequestSeed
    RX: Seed = 0x1234
    
[2] 진단기가 테이블 확인
    0x1234은 어떤 Key와 대응?
    → 0x1234 → 0x4567 ✅
    
[3] SendKey
    TX: Key = 0x4567 (대응값)
    
[4] ECU 검증
    genKey = 0x4567 (RequestSeed에서 설정)
    수신한 key = 0x4567 (진단기가 전송)
    → 일치! 인증 성공 ✅
```

**참고:** 강의록 **P.30 "Table"**: Seed-Key 대응표

---

## 문제 4 정답: CAN 로그 분석

### 4-1 3가지 시도가 모두 성공한 이유

**답:**

```
SecurityAccess_1:
Seed = 0x1234 → 대응 Key = 0x4567 ✅

SecurityAccess_2:
Seed = 0x2345 → 대응 Key = 0x5678 ✅

SecurityAccess_3:
Seed = 0x3456 → 대응 Key = 0x6789 ✅
```

**핵심:**
- 각 Seed마다 정확한 대응 Key가 있음
- 진단기가 테이블을 알고 있으므로 올바른 Key 계산 가능
- ECU의 genKey와 일치 → 인증 성공!

---

### 4-2 잘못된 Key를 보냈을 때

**정답: B) [0x03][0x7F][0x27][0x35][...] - NRC_InvalidKey**

```c
if(key == genKey)  // 0x4567 == 0x5678?
{
	// 성공
}
else
{
	errorResult = NRC_InvalidKey;  // 0x35
}

// NRC 응답:
diagDataLengthTx = 3u;
diagDataBufferTx[0] = 0x03;    // 길이
diagDataBufferTx[1] = 0x7F;    // Negative Response
diagDataBufferTx[2] = 0x27;    // 원본 SID
diagDataBufferTx[3] = 0x35;    // NRC (InvalidKey)
```

**응답 메시지:**

```
[0x03] [0x7F] [0x27] [0x35] [0xAA] [0xAA] [...]
 길이   NegResp SID   NRC
```

**참고:** 강의록 **P.28 코드**: SendKey 실패 케이스

---

## 문제 5 정답: 코드 흐름 추적

### 5-1 모듈로 계산

**답: 1001 % 3 = 1**

```
1001 ÷ 3 = 333.666...
1001 - (333 × 3) = 1001 - 999 = 2

어라? 다시 계산:
1001 ÷ 3 = 333.666...
333 × 3 = 999
1001 - 999 = 2 ❌ 틀렸나?

아니다, 다시:
1001 = 333 × 3 + 2
1001 % 3 = 2 ✓
```

정정: **1001 % 3 = 2**

```
1월 단위:
0ms ~ 999ms:   0 % 3 = 0, 1 % 3 = 1, 2 % 3 = 2
1000ms ~ 1999ms: 1000 % 3 = 1, 1001 % 3 = 2, ...
```

---

### 5-2 시간 1001ms일 때

**답:**

```
UT_getTimeEcuAlive1ms() % 3 = 2

if(... % 3 == 1) ❌
else if(... % 3 == 2) ✅  ← 이 경우
{
	seed = 0x2345;
	genKey = 0x5678;
}
```

따라서: **Seed = 0x2345, genKey = 0x5678**

---

### 5-3 CAN 응답 메시지

**답:**

```
seed = 0x2345

High Byte = (0x2345 & 0xFF00) >> 8 = 0x2300 >> 8 = 0x23
Low Byte = 0x2345 & 0xFF = 0x45

CAN: [0x04][0x67][0x01][0x23][0x45][0xAA]...
```

---

### 5-4 SendKey 검증

**답:**

```
ECU: genKey = 0x5678 (RequestSeed에서 static으로 설정)

진단기: Seed(0x2345)를 받고 테이블 확인
       0x2345 → 0x5678 (대응값)
       
SendKey 요청: TX [0x04][0x27][0x02][0x56][0x78]
                                    ↑    ↑
                                  0x5678

ECU 검증: if(key == genKey)
          if(0x5678 == 0x5678) ✅
          
→ isSecurityUnlock = TRUE ✅
```

---

## 문제 6 정답: 보안 비교

**정답: A, B, C, D 모두 맞음 ✅**

### 해설

**Replay Attack (재현 공격) 방어 메커니즘:**

```
UDS_3의 취약점:
─────────────
공격 시간 T0에 (Seed, Key) = (0x1234, 0x5678) 기록
↓
공격 시간 T1에 같은 값 재사용 시도
→ Seed가 여전히 0x1234이므로 genKey도 0x5678
→ 인증 성공! ❌ 위험!

UDS_4의 방어:
─────────────
공격 시간 T0에 (Seed, Key) = (0x1234, 0x4567) 기록
↓
공격 시간 T1에 같은 값 재사용 시도
→ time_ms가 다르므로 time_ms % 3 다름
→ Seed가 0x2345 or 0x3456으로 변함
→ genKey도 0x5678 or 0x6789으로 변함
→ 수신한 key(0x4567) != genKey → 인증 실패! ✅ 안전!
```

**보안 강화 요소:**

| 요소 | 설명 |
|------|------|
| **시간 기반** | 시간이 지나면 Seed/Key가 변함 |
| **예측 불가능** | ECU 시간을 몰라서 어떤 모드인지 추측 불가 |
| **3가지 모드** | 더 많은 조합 가능성 |
| **Replay 방지** | 과거 로그 재사용 불가 |

**참고:** 강의록 **P.30 "SecurityAccess 1/2/3"**: 세 번 모두 다른 Seed/Key 사용

---

## 📊 채점 기준

| 문제 | 만점 | 난이도 | 핵심 개념 |
|------|-----|--------|---------|
| 1 | 1점 | ⭐⭐⭐⭐ 매우 어려움 | static 변수 이해 |
| 2 | 2점 | ⭐⭐ 중간 | 모듈로 연산, 테이블 |
| 3 | 1점 | ⭐ 쉬움 | Seed-Key 매핑 |
| 4 | 2점 | ⭐⭐ 중간 | CAN 로그 분석 |
| 5 | 4점 | ⭐⭐⭐ 어려움 | 시간 기반 흐름 추적 |
| 6 | 2점 | ⭐⭐⭐ 어려움 | 보안 개념 |
| **합계** | **12점** |  |  |

---

## 🎓 결과 해석

```
11-12점: 완벽! UDS_5/6/7/8로 진행 🚀
9-10점:  우수! 조금 더 보고 진행해도 OK 📖
7-8점:   양호! static 변수와 모듈로 연산 다시 보기 🔄
5-6점:   보통! UDS_4_Analysis의 "2️⃣ 핵심 개념"과 "4️⃣ CAN 로그" 재읽기 🐢
3-4점:   아직도! P.30 강의록과 코드 함께 읽기 📚
0-2점:   다시 한 번! UDS_3부터 차근차근 정리하기 🎓
```

---

## 🔗 UDS 전체 비교 (1~4)

| 항목 | UDS_1 (0x10) | UDS_2 (0x11) | UDS_3 (0x27) | **UDS_4 (0x27)** |
|------|--------------|--------------|--------------|-----------------|
| **명칭** | DiagSessionControl | ECUReset | SecurityAccess | SecurityAccess |
| **특징** | 세션 전환 | 리셋 | 고정 Seed/Key | **테이블 기반** |
| **Seed/Key** | N/A | N/A | 1가지 | **3가지** |
| **선택 방식** | N/A | N/A | 고정값 | **시간 % 3** |
| **보안** | 낮음 | 낮음 | 약함 😅 | **강함 💪** |
| **교육용** | 기초 | 기초 | 기초 | **실무** |
| **상태 변수** | currentSession | isEcuReset | isSecurityUnlock | isSecurityUnlock |

---

**점수 나왔으면 말씀해주세요! 그리고 static 변수가 헷갈린다면 자세히 설명해드릴 수 있습니다.** 😊

**다음:** UDS_5/6/7/8 준비 중입니다! 🚀
