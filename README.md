liboqs-80211
======================

liboqs-80211은 80211-pqc 프로젝트에서 사용하는 암호 라이브러리로, OpenQuantum Safe의 [liboqs](https://github.com/open-quantum-safe/liboqs)를 기반으로 합니다.

새로운 PQC 및 암호 알고리즘을 추가하고 무선·임베디드 환경에 적합하도록 연산을 최적화합니다.
  
## 프로젝트 소개
liboqs-80211은 80211-PQC 프로젝트에서 사용하는 암호 연산들을 다룹니다.
기존 liboqs의 구조와 API를 가능한 한 유지하면서, 무선 공유기와 임베디드 장비에서 PQC 알고리즘을 사용할 수 있도록 새로운 알고리즘 추가와 구현 최적화를 수행합니다.
- KEM, 전자서명 등 PQC 알고리즘을 제공
- 새로운 PQC 및 암호 알고리즘 추가
- ARM, NEON 등 특정 아키텍쳐에 맞는 연산 최적화
- 메모리 사용량과 연산량을 줄이기 위한 최적화

## 프로젝트 구성

80211-PQC 프로젝트는 다음 4개 레포로 구성됩니다.

| 레포 | 역할 |
|------|------|
| 프로토콜 명세서 | 제안 및 표준 기반 상세 구현 명세 |
| [hostap-pqc](https://github.com/80211-PQC/hostap-pqc) | 프로토콜 실 구현체 |
| [openwrt-pqc](https://github.com/80211-PQC/openwrt-pqc) | 구현된 프로토콜을 적용한 펌웨어 |
| **liboqs-80211** *(현재 레포)* | 802.11 환경에 맞게 최적화된 liboqs 연산 구현체 |

## 상태
> ⚠️ **실험 / 연구용 (Experimental)**
> 본 프로젝트는 양자내성암호 구현 및 최적화를 목적으로 합니다.
> 일부 알고리즘과 최적화 구현은 충분한 보안 검증이 완료되지 않았을 수 있으므로 실제 제품 환경에서는 사용하지 마세요.
> API, 지원 알고리즘 및 구현 구조는 개발 과정에서 변경될 수 있습니다.

## 주요 개발 범위
#### PQC 알고리즘 추가
기존 liboqs에서 제공하는 알고리즘 외에도 새로운 PQC 또는 암호 알고리즘 추가할 수 있습니다.
주요 대상은 다음과 같습니다.
- PQC 알고리즘
- 무선 프로토콜을 위한 암호
새로운 알고리즘을 추가할 경우 가능한 기존 libqos의 API 및 디렉터리 구조를 유지합니다.

#### 암호 연산 최적화
무선 공유기와 임베디드 장치는 일반적인 PC나 서버보다 CPU와 메모리 자원이 제한적이므로, 본 프로젝트에서는 다음과 같은 최적화를 주로 개발 대상으로 합니다.
- 모듈로 연산 최적화
- 특정 아키텍쳐 대상 최적화
- 메모리 사용량 감소
- etc..
최적화 구현은 기존 참조 구현과 동일한 암호학적 결과를 생성해야 합니다.

## 기반 (Upstream)
- **원본 프로젝트:** Open Quantum Safe-liboqs — <https://github.com/open-quantum-safe/liboqs>
- **원본 라이선스:** MIT 2.0 License( [liboqs](https://github.com/open-quantum-safe/liboqs)는 해당 라이선스를 유지하지만 사용되는 일부 암호의 라이선스는 다를 수 있으니 해당 페이지를 통해 확인하세요.)


## 빌드 & 설치
### 1. 의존성 설치

Ubuntu에서는 다음 명령어를 실행합니다.

```bash
sudo apt install astyle cmake gcc ninja-build libssl-dev python3-pytest python3-pytest-xdist unzip xsltproc doxygen graphviz python3-yaml valgrind
```

macOS에서는 원하는 패키지 관리자를 사용할 수 있으며, 아래는 Homebrew를 사용하는 예시입니다.

```bash
brew install cmake ninja openssl@3 wget doxygen graphviz astyle valgrind
pip3 install pytest pytest-xdist pyyaml
```

Nix를 사용하는 경우 다음 명령어를 실행합니다.

```bash
nix develop
```

liboqs에서 AES, SHA-2 등의 대칭키 암호 알고리즘 구현에 OpenSSL을 사용하려면 OpenSSL이 설치되어 있어야 합니다.
OpenSSL 3.x 버전 사용을 권장하며, 지원이 종료된 1.1.1 버전도 사용할 수 있습니다.


### 2. 소스 코드 다운로드 및 빌드

다음 명령어를 사용하여 소스 코드를 가져옵니다.

```bash
git clone -b main https://github.com/open-quantum-safe/liboqs.git
cd liboqs
```

이후 다음과 같이 빌드합니다.

```bash
mkdir build && cd build
cmake -GNinja ..
ninja
```

빌드 결과를 구성하기 위한 다양한 `cmake` 옵션을 사용할 수 있으며, 자세한 내용은 [`CONFIGURE.md`](CONFIGURE.md#options-for-configuring-liboqs-builds)를 참고하십시오.

지원되는 모든 옵션은 `.CMake/alg-support.cmake` 파일에서도 확인할 수 있습니다.

또한 `build` 디렉터리에서 다음 명령어를 실행하여 사용 가능한 CMake 옵션을 확인할 수 있습니다.

```bash
cmake -LAH -N ..
```

이후 설명에서는 현재 위치가 `build` 디렉터리라고 가정합니다.


### 3. 빌드 결과 및 테스트

기본적으로 생성되는 주요 빌드 결과는 다음 정적 라이브러리입니다.

```text
lib/liboqs.a
```

공유 라이브러리 또는 동적 라이브러리를 생성하려면 앞서 사용한 CMake 명령어에 다음 옵션을 추가합니다.

```text
-DBUILD_SHARED_LIBS=ON
```

예:

```bash
cmake -GNinja -DBUILD_SHARED_LIBS=ON ..
```

이 경우 플랫폼에 따라 다음과 같은 공유 라이브러리가 생성됩니다.

```text
lib/liboqs.so
lib/liboqs.dylib
lib/liboqs.dll
```

공개 헤더 파일은 `include` 디렉터리에 위치합니다.

전체 테스트는 다음 명령어로 실행할 수 있습니다.

```bash
ninja run_tests
```


### 4. API 문서 생성

API의 HTML 문서를 생성하려면 다음 명령어를 실행합니다.

```bash
ninja gen_docs
```

생성된 문서는 다음 파일을 웹 브라우저에서 열어 확인할 수 있습니다.

```text
docs/html/index.html
```


### 5. 설치

빌드된 라이브러리와 `include` 파일을 시스템에 설치하려면 다음 명령어를 실행합니다.

```bash
ninja install
```

설치 위치를 지정하려면 CMake 설정 단계에서 다음 옵션을 사용할 수 있습니다.

```text
-DCMAKE_INSTALL_PREFIX=<dir>
```

예:

```bash
cmake -GNinja -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

또는 다음 명령어를 사용하여 설치 패키지를 생성할 수 있습니다.

```bash
ninja package
```

### 6. 삭제

설치된 파일을 제거하려면 다음 명령어를 실행합니다.

```bash
ninja uninstall
```

### Windows

Windows에서는 [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) 확장이 설치된 Visual Studio 2019를 사용하여 바이너리를 생성할 수 있습니다.

Linux 및 macOS에서 사용하는 것과 동일한 CMake 옵션을 사용할 수 있으며, 빌드 결과는 지정한 `build` 디렉터리에 생성됩니다.

Ninja를 사용하지 않고 Visual Studio용 빌드 파일을 생성하려면 CMake 명령어에서 `-GNinja` 옵션을 사용하지 않아야 합니다.

이후 `msbuild`를 사용하여 전체 프로젝트를 빌드할 수 있습니다.

```bash
msbuild ALL_BUILD.vcxproj
```

설치하려면 다음 명령어를 사용할 수 있습니다.

```bash
msbuild INSTALL.vcxproj
```

---

### Cross Compilation

liboqs는 다양한 플랫폼을 대상으로 Cross Compilation을 지원합니다.

자세한 내용은 [liboqs Wiki의 플랫폼별 빌드 및 Cross Compilation 문서](https://github.com/open-quantum-safe/liboqs/wiki/Platform-specific-notes-for-building-liboqs#cross-compiling)를 참고하세요.

## 라이선스
본 프로젝트는 원본 liboqs의 라이선스 정책을 따릅니다.

원본 liboqs는 MIT License로 배포됩니다.

- 원본 프로젝트의 저작권 표시 및 라이선스를 유지합니다.
- 외부에서 가져온 개별 알고리즘 구현은 해당 구현의 라이선스 조건을 따를 수 있습니다.
- 자세한 내용은 레포 내 LICENSE, LICENSE.txt 및 각 소스 파일의 라이선스 헤더를 참조하세요.

## 보안
보안 취약점은 공개 이슈로 등록하지 마세요.
취약점 제보 절차와 정책은 [`SECURITY.md`](./SECURITY.md)를 참조하세요.

- 제보 메일 : `wwsddrf15102@gmail.com`

## 기여

기여를 환영합니다. 
기여 범위 구분, 워크플로우, 커밋 규칙은 [`CONTRIBUTING.md`](./CONTRIBUTING.md)를 참조하세요.

- 방식: **Fork & Pull Request**
- 모든 커밋에 **`Signed-off-by`** 서명 필요 (DCO)
- 성능 최적화 PR에는 가능한 한 Benchmark 결과를 포함해 주세요.
- 일반적인 liboqs 자체의 버그 또는 범용 개선 사항은 원본 프로젝트[liboqs](https://github.com/open-quantum-safe/liboqs)에 제보해 주세요.
