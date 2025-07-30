# BlueBuzz

BlueBuzz는 Bluetooth 게임패드를 MSX 스타일 디지털 입력으로 변환하는 Arduino 프로젝트입니다. Bluepad32 라이브러리를 활용하여 Bluetooth 컨트롤러의 입력을 MSX 조이스틱 인터페이스에 맞게 디지털 핀으로 매핑합니다.

## 주요 기능

- Bluetooth 게임패드 연결 및 입력 처리
- MSX 조이스틱 신호(DPAD, A/B 버튼 등)로 매핑
- A/B 버튼 터보(연타) 기능 및 속도 조절
- 터보 속도 조절 시 진동 피드백 제공
- 최대 연결 컨트롤러 수 제한
- Bluepad32 기반, 가상 디바이스 비활성화

## 핀 배치

| 기능      | 핀 번호 |
|-----------|--------|
| STROBE    | D0     |
| UP        | D1     |
| DOWN      | D2     |
| LEFT      | D3     |
| RIGHT     | D4     |
| A 버튼    | D5     |
| B 버튼    | D6     |

## 사용법

1. Arduino에 BlueBuzz.ino를 업로드합니다.
2. Bluetooth 게임패드를 페어링하여 연결합니다.
3. 게임패드 입력이 MSX 조이스틱 핀에 디지털 신호로 출력됩니다.
4. 어깨/트리거 버튼으로 터보 속도를 조절할 수 있습니다.
5. 터보 속도 조절 시 컨트롤러에 진동 피드백이 발생합니다.

## 의존성

- [Bluepad32](https://github.com/ricardoquesada/bluepad32) 라이브러리

## 라이선스

MIT License

## 참고

- MSX 조이스틱 인터페이스에 대한 자세한 정보는 [MSX Wiki](https://www.msx.org/wiki/Joystick_Port) 참고
- Bluepad32 공식 문서 및 예제 활용