# 기여 가이드 (Contributing)

`liboqs-80211`에 관심 가져주셔서 감사합니다.  
이 문서는 기여 범위, 절차, 커밋 규칙 및 암호 연산 최적화 기여 시 필요한 검증 사항을 안내합니다.

기여하기 전에 **[기여 범위](#기여-범위)** 를 먼저 확인해 주세요.  
변경의 성격에 따라 제출해야 할 위치가 달라집니다.

> ℹ️ `liboqs-80211`은 Open Quantum Safe의 [liboqs](https://github.com/open-quantum-safe/liboqs)를 기반으로 합니다.  
> 일반적인 liboqs 자체의 버그 또는 범용 개선 사항은 가능하면 원본 liboqs 프로젝트에 제출해 주세요.

## 기여 범위

`liboqs-80211`은 80211-PQC 프로젝트에서 사용하는 PQC 및 암호 연산을 제공하고, 이를 무선 공유기와 임베디드 환경에 적합하도록 최적화하는 레포입니다.

변경의 성격에 따라 제출 위치가 다르므로 아래 표를 먼저 확인해 주세요.

| 변경 성격 | 제출 위치 |
|-----------|-----------|
| PQC 암호 연산 구현 및 최적화 | `liboqs-80211` |
| ARM, NEON 등 특정 아키텍처 최적화 | `liboqs-80211` |
| 메모리·스택 사용량 감소 및 연산량 최적화 | `liboqs-80211` |
| OpenWrt 환경에 종속된 PQC 패키지 통합 | [openwrt-pqc](https://github.com/80211-pqc/openwrt-pqc) |
| hostapd/wpa_supplicant의 PQC 프로토콜 구현 | [hostap-pqc](https://github.com/80211-pqc/hostap-pqc) |
| 프로토콜 설계·명세 관련 논의 | [protocol](https://github.com/80211-pqc/protocol) |
| liboqs 자체의 범용 버그·기능 개선 | 원본 프로젝트 [liboqs](https://github.com/open-quantum-safe/liboqs) |

> ℹ️ 특정 공유기나 802.11 환경에 종속되지 않고 범용적으로 적용할 수 있는 liboqs 개선 사항은 원본 liboqs 프로젝트에 제출하는 것을 권장합니다.

## 기여 유형

이 레포에서 환영하는 기여는 다음과 같습니다.

- **암호 연산 최적화** — ML-KEM/Kyber, NTT/INTT, modular reduction, polynomial 연산 등
- **아키텍처 최적화** — ARM, ARMv8-A, NEON 등 특정 CPU 및 명령어 집합을 활용한 최적화
- **메모리 최적화** — stack, heap, temporary buffer 사용량 감소
- **임베디드 최적화** — 제한된 CPU·메모리 환경을 고려한 구현 개선
- **새로운 암호 구현 연동** — liboqs의 구조와 API를 유지하는 범위에서 필요한 PQC 알고리즘 연동
- **버그 수정** — 암호 연산 오류, 빌드 실패, 플랫폼 호환성 문제
- **테스트 및 Benchmark 개선** — correctness test, benchmark, memory 측정 등
- **문서 개선** — README, CONTRIBUTING, 빌드 문서, 코드 주석 등
- **보안 취약점 제보** — 아래 주의사항 참고

> ⚠️ **보안 취약점은 공개 Issue로 등록하지 마십시오.**  
> 암호 구현 또는 최적화 과정에서 발견된 취약점은 비공개로 제보해야 합니다.  
> 자세한 절차는 [`SECURITY.md`](./SECURITY.md)를 참고해 주세요.

## 기여 절차

이 프로젝트는 **Fork & Pull Request** 방식으로 기여를 받습니다.

1. 이 레포를 **Fork**합니다.
2. 작업용 브랜치를 만듭니다.
3. 변경 사항을 구현합니다.
4. 테스트 및 필요한 경우 benchmark를 수행합니다.
5. **`Signed-off-by`를 포함하여** 커밋합니다.
6. Fork한 레포에 push합니다.
7. 이 레포로 **Pull Request**를 생성합니다.
8. 리뷰 의견에 따라 수정하고, 승인되면 병합됩니다.

예시 브랜치:

```text
feat/ml-kem-support
opt/neon-ntt
opt/reduce-stack
fix/decapsulation-error
docs/update-build-guide
```

> ℹ️ 암호 핵심 연산 변경, 새로운 아키텍처 최적화, API 변경 등 영향 범위가 큰 작업은 PR 전에 Issue를 통해 먼저 논의해 주세요.

## 커밋 규칙

### Signed-off-by (DCO)

모든 커밋에는 `Signed-off-by` 서명이 **필수**입니다.

커밋 시 `-s` 옵션을 사용하면 자동으로 추가됩니다.

```bash
git commit -s -m "커밋 메시지"
```

### 커밋 메시지

제목은 간결하게 작성하고, 본문에는 무엇을 왜 변경했는지 설명해 주세요.

예:

```text
perf: optimize NTT for ARMv8-A

opt: reduce temporary buffers in ML-KEM decapsulation

fix: resolve ML-KEM shared secret mismatch

test: add ML-KEM768 benchmark

docs: update ARM build instructions
```

성능 및 최적화 관련 변경은 가능하면 다음 내용을 커밋 또는 PR 본문에 포함해 주세요.

- 대상 architecture
- 대상 CPU 또는 SoC
- 사용한 compiler
- 최적화 방법
- 변경 전후 성능

## 암호 구현 및 최적화 원칙

암호 연산의 경우 성능보다 정확성과 보안이 우선합니다.

### 암호 파라미터 임의 변경 금지

성능 향상을 목적으로 다음과 같은 암호 파라미터를 임의로 변경하지 마세요.

- polynomial degree
- modulus
- noise distribution
- compression parameter
- public key size
- secret key size
- ciphertext size

실험적인 파라미터 변경이 필요한 경우 반드시 Issue에서 먼저 논의하고, 기존 구현과 명확히 분리해야 합니다.

### Constant-Time 특성 유지

암호 구현을 최적화할 때 다음과 같은 보안 문제가 발생하지 않도록 주의해 주세요.

- secret-dependent branch
- secret-dependent memory access
- timing side channel
- secret key leakage
- randomness 품질 저하
- 비정상적인 ciphertext 처리

암호 구현의 보안 특성을 훼손하는 성능 최적화는 병합되지 않습니다.

### Architecture-Specific Optimization

ARM, NEON 등 특정 architecture를 위한 최적화를 추가할 수 있습니다.

예:

```text
ARM Cortex-A53
ARMv8-A
NEON
```

architecture-specific implementation을 추가한 경우 가능한 한 기존 portable implementation을 fallback으로 유지해 주세요.

특정 CPU instruction을 사용할 경우 해당 명령어를 지원하지 않는 환경에서 기존 구현을 사용할 수 있어야 합니다.

### 성능 측정

성능 개선을 목적으로 한 PR은 가능한 한 변경 전후 benchmark 결과를 포함해 주세요.

권장 측정 항목은 다음과 같습니다.

```text
Key Generation
Encapsulation
Decapsulation
CPU Cycles
Stack Usage
Heap Usage
Peak Memory
Binary Size
```

예:

```text
Environment:
Device:
SoC:
CPU:
Architecture:
Compiler:
Compiler Flags:

Before:
KeyGen:
Encapsulation:
Decapsulation:
Stack:
Binary Size:

After:
KeyGen:
Encapsulation:
Decapsulation:
Stack:
Binary Size:
```

단일 실행 결과보다는 여러 번 반복 측정한 결과를 사용하는 것을 권장합니다.

## 새로운 암호 알고리즘 추가

새로운 암호 알고리즘을 추가할 경우 가능한 한 원본 liboqs의 API 및 구조를 따릅니다.

본 프로젝트에서 독립적인 암호 API를 새로 정의하거나 기존 liboqs 구조와 호환되지 않는 형태로 구현하는 것은 지양합니다.

## 코드 스타일

기존 liboqs의 코드 스타일과 디렉터리 구조를 최대한 따릅니다.

특히 다음 사항을 지켜 주세요.

- 주변 코드와 스타일을 일관되게 유지
- 기존 liboqs API 구조 유지
- architecture-specific 코드에는 대상 환경 명시
- 최적화 코드에는 필요한 경우 구현 의도 설명
- 원본 저작권 및 라이선스 헤더 유지

새 파일을 추가할 때는 해당 코드의 라이선스에 맞는 SPDX header를 포함해야 합니다.

기존 liboqs에서 가져온 코드를 수정하는 경우 기존 copyright 및 license 정보를 삭제하지 마세요.

## Pull Request 체크리스트

PR을 생성하기 전에 아래 항목을 확인해 주세요.

- [ ] 프로젝트가 정상적으로 빌드됨
- [ ] `ninja run_tests`가 통과함
- [ ] 연산 결과가 정상적으로 일치함
- [ ] 기존 기능에 regression이 없음
- [ ] 모든 커밋에 `Signed-off-by`가 포함됨
- [ ] 기존 저작권 및 라이선스 정보가 유지됨
- [ ] 새 파일에 적절한 SPDX header가 포함됨
- [ ] architecture-specific 변경의 대상 환경을 명시함
- [ ] 성능 최적화 PR에는 가능한 한 benchmark 결과를 포함함


## 리뷰 및 병합

제출된 PR은 프로젝트 관리자의 리뷰를 거쳐 병합됩니다.

특히 암호 구현 또는 성능 최적화 관련 PR은 다음 순서로 검토합니다.

1. Cryptographic Correctness
2. Security
3. Compatibility
4. Stability
5. Performance
6. Code Quality

성능이 향상되더라도 암호학적 정확성이나 보안성을 훼손하는 변경은 병합되지 않습니다.

특정 architecture에서 성능이 향상되더라도 다른 지원 환경의 동작을 불필요하게 깨뜨리는 변경은 수정 요청될 수 있습니다.

> ℹ️ 현재 별도의 관리자 조직이 구성되기 전까지는 프로젝트 오너가 리뷰를 담당합니다.

## 라이선스 및 DCO

`liboqs-80211`은 원본 liboqs의 라이선스 정책을 따릅니다.

원본 liboqs는 **MIT License**로 배포되지만, 포함된 개별 암호 알고리즘 구현은 서로 다른 라이선스를 사용할 수 있습니다.

모든 커밋에는 `Signed-off-by`를 포함해야 하며, 이를 통해 기여자는 [DCO](https://developercertificate.org/)에 동의하는 것으로 간주됩니다.
