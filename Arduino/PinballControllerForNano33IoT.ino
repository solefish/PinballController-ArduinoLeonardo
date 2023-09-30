#include <Keyboard.h>
#include <Arduino_LSM6DS3.h>

//------------------------------------------------------------------------------
// Pinballコントローラー for Arduino Nano 33 Iot
//                                                          したびらめ(Solefish)
//                                                    http://shitabirame.sub.jp/
//------------------------------------------------------------------------------
// 履歴
// ver 1.0  2013/07/03  初出
// ver 1.1  2020/03/25  キーボードライブラリを明示的にインクルードするよう修正
// ver 2.0  2023/09/26  Arduino Nano 33 Iotへ移行
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// キー割り当て
//------------------------------------------------------------------------------
// ゲームのキー設定をここに記述してください。
//   割り振りたいキーをシングルクォーテーションで囲って記述します。（例：'1'）
//   英字キーを割り当てるときは、小文字で指定します。（例：'f'）
//   特殊キーを割り当てるときは、Arduinoスケッチ用定数を記述してください。
//     https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/
// 注意：右SHIFTキーなど一部のキーはゲームが認識できない事があります。
//       その場合はあらかじめゲームのキー設定で認識できるキーに設定を変更してください。

//      定義名      送信するキー           機能
#define KB_ST       KEY_ESC             // ポーズ
#define KB_PL       KEY_RETURN          // プランジャー
#define KB_FL       KEY_LEFT_SHIFT      // 左フリッパー
#define KB_FR       KEY_RIGHT_SHIFT     // 右フリッパー
#define KB_NL       KEY_RIGHT_CTRL      // 左ナッジ（左右逆）
#define KB_NR       KEY_LEFT_CTRL       // 右ナッジ（左右逆）
#define KB_NU       ' '                 // 上ナッジ

//------------------------------------------------------------------------------
// ナッジ（アナログ）の設定
//------------------------------------------------------------------------------

// 加速度センサーの初期値
// 加速度センサーが水平で停止している時のAD値を設定します。変更しないでください。
#define SENSE_CENTER_X        0         // 加速度センサーの水平時X値
#define SENSE_CENTER_Y        0         // 加速度センサーの水平時Y値

// ナッジ検出しきい値
// 加速度センサーがどれだけ振れたときにナッジが起きるか設定します。
// 0～511の整数を設定してください。値が大きいほどナッジを検出しにくくなります。
#define SENSE_NUDGE_X       1.5         // 横ナッジ検出値
#define SENSE_NUDGE_Y       1.5         // 縦ナッジ検出値

// ナッジキーホールド時間
// ナッジを検出した時、ナッジキーを送信する時間(ミリ秒)を設定します。
// 1以上の整数を設定してください。時間が短かすぎると反応しない場合があります。
#define NUDGE_TIMESPAN       10         // ナッジキーを押し続けるフレーム数

//------------------------------------------------------------------------------
// Arduino入力端子の定義
//------------------------------------------------------------------------------
// 入力端子を変えたときは、ここの設定を変更してください。

//      スケッチ内の名前    端子番号
#define PI_START            A0          // ポーズボタン入力端子
#define PI_PLUNGER          A1          // プランジャー入力端子
#define PI_FLIPPER_LEFT     A2          // 左フリッパー入力端子
#define PI_FLIPPER_RIGHT    A3          // 右フリッパー入力端子

//------------------------------------------------------------------------------
// グローバル変数
//------------------------------------------------------------------------------

// キーバッファ
unsigned char KEY_BUFFER[256];                    // キーバッファ
unsigned char KEY_BUFFER_OLD[256];                // 前フレームのキーバッファ

//------------------------------------------------------------------------------
// 初期設定
//------------------------------------------------------------------------------
void setup() {
  
  // デジタル入力ピン設定
  // ボタンに繋がるピンを入力に設定し、全てプルアップします。
  pinMode( PI_START        , INPUT_PULLUP );    // スタートボタン入力端子
  pinMode( PI_PLUNGER      , INPUT_PULLUP );    // プランジャー入力端子
  pinMode( PI_FLIPPER_LEFT , INPUT_PULLUP );    // 左フリッパー入力端子
  pinMode( PI_FLIPPER_RIGHT, INPUT_PULLUP );    // 右フリッパー入力端子
  
  // 加速度センサ初期化
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // キーボード機能初期化
  Keyboard.begin();
  
  // 前フレームのキーバッファクリア
  for(int i=0; i<256; i++ ){
  	KEY_BUFFER_OLD[i] = 0;
  }
  
}

//------------------------------------------------------------------------------
// メインループ
//------------------------------------------------------------------------------
void loop() {
  
  // 変数宣言
  static int NudgeBuff_Left  = 0;                        // 左ナッジ残りフレーム数
  static int NudgeBuff_Right = 0;                        // 右ナッジ残りフレーム数
  static int NudgeBuff_Up    = 0;                        // 上ナッジ残りフレーム数
  
  // キーバッファクリア
  for(int i=0; i<256; i++ ){
    KEY_BUFFER[i] = 0;
  }
  
  //----------------------------------------------------------------------------
  // アナログ入力の処理
  //----------------------------------------------------------------------------
  
  // アナログ入力読み取り
  float x, y, z;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
  }

  // 左ナッジ（アナログ）
  // ナッジ検出
  if( x > SENSE_CENTER_X + SENSE_NUDGE_X && NudgeBuff_Right == 0 )
    NudgeBuff_Left = NUDGE_TIMESPAN;                    // 左ナッジ残り時間セット
  // ナッジ持続判定
  if( NudgeBuff_Left > 0 ){
    KEY_BUFFER[ KB_NL ] = 1;                            // キーバッファをON
    NudgeBuff_Left--;                                   // 左ナッジ残り時間を減らす
  }
  
  // 右ナッジ（アナログ）
  // ナッジ検出
  if( x < SENSE_CENTER_X - SENSE_NUDGE_X && NudgeBuff_Left == 0 )
    NudgeBuff_Right = NUDGE_TIMESPAN;                   // 右ナッジ残り時間セット
  // ナッジ持続判定
  if( NudgeBuff_Right > 0 ){
    KEY_BUFFER[ KB_NR ] = 1;                            // キーバッファをON
    NudgeBuff_Right--;                                  // 右ナッジ残り時間を減らす
  }
  
  // 上ナッジ（アナログ）
  // ナッジ検出
  if( y < SENSE_CENTER_Y - SENSE_NUDGE_Y )
    NudgeBuff_Up = NUDGE_TIMESPAN;                      // 上ナッジ残り時間セット
  // ナッジ持続判定
  if( NudgeBuff_Up > 0 ){
    KEY_BUFFER[ KB_NU ] = 1;                            // キーバッファをON
    NudgeBuff_Up--;                                     // 上ナッジ残り時間を減らす
  }
  
  //----------------------------------------------------------------------------
  // デジタル入力の処理
  //----------------------------------------------------------------------------
  
  // スタートボタン
  if( digitalRead( PI_START ) == LOW )
    KEY_BUFFER[ KB_ST ] = 1;                            // キーバッファをON
  
  // プランジャーボタン
  if( digitalRead( PI_PLUNGER ) == LOW )
    KEY_BUFFER[ KB_PL ] = 1;                            // キーバッファをON
  
  // 左フリッパーボタン
  if( digitalRead( PI_FLIPPER_LEFT ) == LOW )
    KEY_BUFFER[ KB_FL ] = 1;                            // キーバッファをON
  
  // 右フリッパーボタン
  if( digitalRead( PI_FLIPPER_RIGHT ) == LOW )
    KEY_BUFFER[ KB_FR ] = 1;                            // キーバッファをON
  
  
  //----------------------------------------------------------------------------
  // キーバッファ送出
  //----------------------------------------------------------------------------
  for( int i=0; i<256; i++ ){
    // キーが新たに押された
    if( KEY_BUFFER[i]==1 && KEY_BUFFER_OLD[i]==0 ) Keyboard.press( i );
    // 押されているキーが離された
    if( KEY_BUFFER[i]==0 && KEY_BUFFER_OLD[i]==1 ) Keyboard.release( i );
    
    // 前フレームのキーバッファの更新
    KEY_BUFFER_OLD[i] = KEY_BUFFER[i];
  }
  
  //----------------------------------------------------------------------------
  // 10ミリ秒待つ
  //----------------------------------------------------------------------------
  delay(10);
  
}
