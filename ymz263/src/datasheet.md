# YMZ263B — Multimedia Audio & Game Interface Controller (MMA)

[English translation →](./datasheet.en.md)

<div align="center">

**YMZ263B**

</div>

## 概要

YMZ263B (MMA) は、コンピュータ機器のマルチメディア化に必要な PCM、ADPCM の録音・再生機能、MIDI 通信機能、汎用ゲームポートを 1 チップに集積しています。

YM3812 (OPL2)、YMF262 (OPL3) 等と組み合わせて、マルチメディアパソコンのサウンド機能をコンパクトに実現することが可能です。

## 特徴

### (1) PCM/ADPCM 部

- PCM または ADPCM 方式選択可能。
- 録音・再生可能な 2 チャンネル内蔵。
- サンプリング周波数は、ADPCM モード時は 22.05 kHz、11.025 kHz、7.35 kHz、5.5125 kHz、PCM モード時は 44.1 kHz、22.05 kHz、11.025 kHz、7.35 kHz、の中からチャンネルごとに選択可能。
- PCM は 8 ビットまたは 12 ビット分解能、ADPCM は 12 ビットデータを 4 ビットに圧縮。
- 録音、再生のための、12 ビットフローティング A/D、D/A コンバータ内蔵。
- 2 チャンネルで 2 倍オーバーサンプリングの A/D 変換。
- 4 チャンネルで 2 倍オーバーサンプリングの D/A 変換。
- CPU との音声データ入出力のために、CHANNEL 1、2 各々 128 バイトの FIFO バッファを内蔵し、CPU (ポーリング / インタラプト) モード、DMA モードを選択可能。

### (2) MIDI 部

- MIDI 規格に準拠したデータ送受信のための UART。
- 送受信とも 16 バイト FIFO バッファを内蔵。

### (3) ゲームポート部

- ジョイスティック等とのインターフェイスのための 8 入力ポート。

### (4) その他

- 3 種類のタイマーを内蔵。
- アドレスデコーダ内蔵。
- 5 V 単一、シリコンゲート CMOS プロセス。
- 64 ピンプラスチック QFP。

### ブロック図

![ブロック図](images/block-diagram.png)

## 端子配置図

**YMZ263B-F**

![端子配置図 (YMZ263B-F)](images/pin-layout.png)

<div align="center">

**端子機能**

</div>

<table border="1"><tr><th>No.</th><th>名称</th><th>I/O</th><th>機能</th></tr><tr><td>1</td><td>A1</td><td>I</td><td>CPU インターフェイス アドレスバス</td></tr><tr><td>2</td><td>A2</td><td>I</td><td>アドレスバス</td></tr><tr><td>3</td><td>A3</td><td>I</td><td>アドレスバス</td></tr><tr><td>4</td><td>A4</td><td>I</td><td>アドレスバス</td></tr><tr><td>5</td><td>A5</td><td>I</td><td>アドレスバス</td></tr><tr><td>6</td><td>A6</td><td>I</td><td>アドレスバス</td></tr><tr><td>7</td><td>A7</td><td>I</td><td>アドレスバス</td></tr><tr><td>8</td><td>A8</td><td>I</td><td>アドレスバス</td></tr><tr><td>9</td><td>A9</td><td>I</td><td>アドレスバス</td></tr><tr><td>10</td><td>AEN</td><td>I</td><td>アドレスイネーブル</td></tr><tr><td>11</td><td>VSS</td><td>-</td><td>グランド (デジタル系)</td></tr><tr><td>12</td><td>D0</td><td>I/O</td><td>CPU インターフェイス データバス</td></tr><tr><td>13</td><td>D1</td><td>I/O</td><td>データバス</td></tr><tr><td>14</td><td>D2</td><td>I/O</td><td>データバス</td></tr><tr><td>15</td><td>D3</td><td>I/O</td><td>データバス</td></tr><tr><td>16</td><td>D4</td><td>I/O</td><td>データバス</td></tr><tr><td>17</td><td>D5</td><td>I/O</td><td>データバス</td></tr><tr><td>18</td><td>D6</td><td>I/O</td><td>データバス</td></tr><tr><td>19</td><td>D7</td><td>I/O</td><td>データバス</td></tr><tr><td>20</td><td>DRQ1</td><td>O</td><td>DMA リクエスト信号 1</td></tr><tr><td>21</td><td>/DACK1</td><td>I</td><td>DMA アクノレッジ信号 1</td></tr><tr><td>22</td><td>DRQ2</td><td>O</td><td>DMA リクエスト信号 2</td></tr><tr><td>23</td><td>/DACK2</td><td>I</td><td>DMA アクノレッジ信号 2</td></tr><tr><td>24</td><td>/IRQ</td><td>OD</td><td>CPU インターフェイス 割り込み信号</td></tr><tr><td>25</td><td>/IC</td><td>I+</td><td>イニシャルクリア入力</td></tr><tr><td>26</td><td>VDD</td><td>-</td><td>+5 V 電源 (デジタル系)</td></tr><tr><td>27</td><td>RXD</td><td>I</td><td>MIDI UART データ入力</td></tr><tr><td>28</td><td>TXD</td><td>O</td><td>MIDI UART データ出力</td></tr><tr><td>29</td><td>/ENGP</td><td>O</td><td>アドレスデコーダ出力 ゲームポート部用 (201H)</td></tr><tr><td>30</td><td>/CSGP</td><td>I+</td><td>ゲームポート部 チップセレクト</td></tr><tr><td>31</td><td>GP7</td><td>I+</td><td>ゲームポート部 入力ポート</td></tr><tr><td>32</td><td>GP6</td><td>I+</td><td>ゲームポート部 入力ポート</td></tr><tr><td>33</td><td>GP5</td><td>I+</td><td>ゲームポート部 入力ポート</td></tr><tr><td>34</td><td>GP4</td><td>I+</td><td>ゲームポート部 入力ポート</td></tr><tr><td>35</td><td>GP3</td><td>I+A</td><td>ゲームポート部 入力ポート</td></tr><tr><td>36</td><td>GP2</td><td>I+A</td><td>ゲームポート部 入力ポート</td></tr><tr><td>37</td><td>GP1</td><td>I+A</td><td>ゲームポート部 入力ポート</td></tr><tr><td>38</td><td>GP0</td><td>I+A</td><td>ゲームポート部 入力ポート</td></tr><tr><td>39</td><td>RV</td><td>I+A</td><td>ゲームポート部 被比較電圧入力</td></tr><tr><td>40</td><td>AVSS</td><td>-A</td><td>グランド (アナログ系)</td></tr><tr><td>41</td><td>CSH2</td><td>I+A</td><td>A/D 変換用 サンプルホールド容量接続端子 2</td></tr><tr><td>42</td><td>AIN2</td><td>I+A</td><td>アナログ入力 2</td></tr><tr><td>43</td><td>R2</td><td>OA</td><td>チャンネル 2 R 出力</td></tr><tr><td>44</td><td>L2</td><td>OA</td><td>チャンネル 2 L 出力</td></tr><tr><td>45</td><td>CSH1</td><td>I+A</td><td>A/D 変換用 サンプルホールド容量接続端子 1</td></tr><tr><td>46</td><td>AIN1</td><td>I+A</td><td>アナログ入力 1</td></tr><tr><td>47</td><td>R1</td><td>OA</td><td>チャンネル 1 R 出力</td></tr><tr><td>48</td><td>L1</td><td>OA</td><td>チャンネル 1 L 出力</td></tr><tr><td>49</td><td>CV</td><td>OA</td><td>A/D 変換器センター電圧端子</td></tr><tr><td>50</td><td>AVSS</td><td>-A</td><td>グランド (アナログ系)</td></tr><tr><td>51</td><td>AVDD</td><td>-A</td><td>+5 V 電源 (アナログ系)</td></tr><tr><td>52</td><td>CH 1 FS 0</td><td>O</td><td>PCM/ADPCM CHANNEL 1 サンプリング周波数情報出力 0</td></tr><tr><td>53</td><td>CH 1 FS 1</td><td>O</td><td>サンプリング周波数情報出力 1</td></tr><tr><td>54</td><td>CH 2 FS 0</td><td>O</td><td>PCM/ADPCM CHANNEL 2 サンプリング周波数情報出力 0</td></tr><tr><td>55</td><td>CH 2 FS 1</td><td>O</td><td>サンプリング周波数情報出力 1</td></tr><tr><td>56</td><td>XO</td><td>O</td><td>水晶発振子接続端子</td></tr><tr><td>57</td><td>XI</td><td>I</td><td>水晶発振子接続端子 またはマスタークロック入力 (16.9344 MHz)</td></tr><tr><td>58</td><td>VDD</td><td>-</td><td>+5 V 電源 (デジタル系)</td></tr><tr><td>59</td><td>/EN 2</td><td>O</td><td>アドレスデコーダ出力 OPL3 等音源用 (388H〜38BH)</td></tr><tr><td>60</td><td>/EN 1</td><td>O</td><td>MMA (ゲームポート部除く) 用 (38CH〜38FH)</td></tr><tr><td>61</td><td>/CS</td><td>I+</td><td>CPU インターフェイス チップセレクト</td></tr><tr><td>62</td><td>/WR</td><td>I</td><td>ライトイネーブル</td></tr><tr><td>63</td><td>/RD</td><td>I</td><td>リードイネーブル</td></tr><tr><td>64</td><td>A0</td><td>I</td><td>アドレスバス</td></tr></table>

注) I/O 欄の記号:

- **OD** : オープンドレイン出力端子
- **I+** : プルアップ抵抗内蔵入力端子
- **A** : アナログ信号端子
- **I+A** : プルアップ抵抗内蔵アナログ入力端子 (通常は AVSS へショートされています)

## 機能説明

### 1. クロック生成 XI, XO

XI, XO 端子を使用して水晶発振回路を構成します。発振周波数は、16.9344 MHz です。

XI 端子に外部よりクロックを入力してもかまいません。

### 2. CPU インターフェイス A0, A1, D0〜7, /CS, /RD, /WR, /IRQ

本 LSI 各部のコントロールのために 8 ビットパラレルインターフェイスが用意されています。

レジスタデータのリード・ライト、ステータスリードなどのデータバスコントロールは、/CS、/RD、/WR、A0、A1 の各信号で行います。これらの信号により、データバスは以下のようなモードとなります。

<table border="1"><tr><td>/CS</td><td>/RD</td><td>/WR</td><td>A0</td><td>A1</td><td>CPU アクセスモード</td></tr><tr><td>H</td><td>×</td><td>×</td><td>×</td><td>×</td><td>インアクティブモード</td></tr><tr><td>L</td><td>H</td><td>L</td><td>L</td><td>×</td><td>アドレスライトモード</td></tr><tr><td>L</td><td>H</td><td>L</td><td>H</td><td>L/H</td><td>データライトモード</td></tr><tr><td>L</td><td>L</td><td>H</td><td>L</td><td>L</td><td>ステータスリードモード</td></tr><tr><td>L</td><td>L</td><td>H</td><td>H</td><td>L/H</td><td>データリードモード</td></tr></table>

注) × (don't care)

**(a) インアクティブモード**

/CS が 'H' の時、データバス D0〜D7 はハイインピーダンスとなります。

**(b) アドレスライトモード**

書き込み、読み出しするレジスタのアドレスを指定するモードです。データバスにはアドレスデータをセットします。

**(c) データライトモード**

アドレスライトモードで設定されたアドレスにデータを書き込むモードです。データバスのデータが指定されたアドレスのレジスタに書き込まれます。

**(d) ステータスリードモード**

ステータス情報を読み出すモードです。データバスにはステータス情報が出力されます。

**(e) データリードモード**

アドレスライトモードで設定されたアドレスからデータを読み出すモードです。データバスには指定されたアドレスのレジスタのデータが出力されます。

本 LSI の各部から割り込み信号が発生すると、/IRQ 端子を 'L' として CPU へ通知します。

注) YMZ263B では、書き込みから次の書き込み動作に移るまでに、あるいは読み出しから次の読み出し動作に移るまでに、以下のウェイト時間が必要です。

> ウェイト時間 ... 8 サイクル (マスタークロック) 以上

### 3. FIFO 部 DRQ1, DRQ2, /DACK1, /DACK2

PCM/ADPCM 部と CPU とのデータの入出力は CHANNEL 1、2 各々 128 バイトの FIFO を介して行います。DMA コントローラと接続し、DMA 転送を行う事も可能です。

### 4. PCM/ADPCM 部

CH1FS1, CH1FS0, L1, R1, AIN1, CSH1, CV / CH2FS1, CH2FS0, L2, R2, AIN2, CSH2

PCM/ADPCM デコーダ出力は、CHANNEL 1、2 の 2 チャンネル各々のデジタルボリュームによって出力レベルを調整され、2 倍オーバーサンプリング処理後、設定されたサンプリング周波数の 2 倍の周波数で D/A 変換され、L1、R1、L2、R2 各端子より電圧出力されます。(ただし、44.1 kHz の PCM モードではオーバーサンプリング処理は行いません。)

AIN1, AIN2 より入力されるアナログ信号は、設定されたサンプリング周波数の 2 倍の周波数で A/D 変換され、1/2 倍アンダーサンプリング処理されて PCM/ADPCM エンコーダに入力されます。(ただし、44.1 kHz の PCM モードではアンダーサンプリング処理は行いません。)

CSH1, CSH2 端子には A/D 変換のためのサンプルホールド容量を外付けします。CV 端子は A/D 変換器のセンター電圧端子です。

CH1FS1,0, CH2FS1,0 からは外部 LPF 切り替え等のために、各々 CHANNEL 1、CHANNEL 2 の PCM/ADPCM サンプリング周波数情報を出力します。

<table border="1"><tr><td>PCM</td><td>ADPCM</td><td>CH1FS1, CH2FS1</td><td>CH1FS0, CH2FS0</td></tr><tr><td>44.1 kHz</td><td>-</td><td>L</td><td>L</td></tr><tr><td>22.05 kHz</td><td>22.05 kHz</td><td>L</td><td>H</td></tr><tr><td>11.025 kHz</td><td>11.025 kHz</td><td>H</td><td>L</td></tr><tr><td>7.35 kHz</td><td>7.35 or 5.5125 kHz</td><td>H</td><td>H</td></tr></table>

### 5. MIDI 部 TXD, RXD

送信データは 16 バイトの FIFO でバッファリングされ、UART より TXD 端子から調歩同期出力されます。RXD 端子より入力される調歩同期入力は UART により受信され 16 バイト FIFO によりバッファリングされます。

### 6. ゲームポート部 /CSGP, GP0〜7, RV

RV 端子には被比較電圧 ($0.63 \times V_{DD} \sim 0.70 \times V_{DD}$) を入力して下さい。

GP0〜3 端子は、通常内部で AVSS へショートしていますので注意が必要です。

/CSGP = 'L'、/WR = 'L' で GP0〜3 端子は AVSS 端子と切り離され、被比較電圧より GP0〜3 の端子電圧が高くなると、フリップフロップが 0 にリセットされます。フリップフロップの値は /CSGP = 'L'、/RD = 'L' で各々 D0〜3 端子より読み出すことができます。従って外部の時定数に応じて 0 にリセットされるまでの時間は変化します。

GP4〜7 端子は汎用入力ポートで、/CSGP = 'L'、/RD = 'L' で GP4〜7 の値が各々データバス D4〜7 へ出力されます。

![GP0〜7 端子 入力等価回路と外付回路例](images/game-port-circuit.png)

### 7. アドレスデコーダ部 AEN, A0〜9, /EN1, /EN2, /ENGP

外付け回路削減のため固定値のアドレスデコードを内蔵しています。

/EN1、/EN2、/ENGP は各々、YMZ263B (ゲームポートを除く)、YMF262 等の音源、ゲームポート部へのアドレスデコーダ出力でアドレスが一致すると 'L' となります。

AEN は DMA 動作時にチップセレクトの誤発生を防ぐために使用します。AEN = 'H' の時は A0〜9 がどんな値でも /EN1、/EN2、/ENGP は 'L' にはなりません。

### 8. イニシャルクリア

本 LSI は電源投入時にイニシャルクリアが必要です。

## レジスタ説明

<div align="center">

### 1. レジスタマップ

</div>

<table border="1"><tr><td>CH</td><td colspan="9">CHANNEL 1 (A1='L')</td><td colspan="9">CHANNEL 2 (A1='H')</td></tr><tr><td>ADDR</td><td>R/W</td><td>D7</td><td>D6</td><td>D5</td><td>D4</td><td>D3</td><td>D2</td><td>D1</td><td>D0</td><td>R/W</td><td>D7</td><td>D6</td><td>D5</td><td>D4</td><td>D3</td><td>D2</td><td>D1</td><td>D0</td></tr><tr><td>$00</td><td>R/W</td><td colspan="7"></td><td>SELT</td><td>—</td><td rowspan="9" colspan="9"></td></tr><tr><td>$01</td><td>—</td><td colspan="8">LSI TEST</td><td>—</td></tr><tr><td>$02</td><td>W</td><td colspan="8">TIMER 0 (L)</td><td>—</td></tr><tr><td>$03</td><td>W</td><td colspan="8">TIMER 0 (H)</td><td>—</td></tr><tr><td>$04</td><td>W</td><td colspan="8">BASE COUNTER (L)</td><td>—</td></tr><tr><td>$05</td><td>W</td><td colspan="4">TIMER 1</td><td colspan="4">BASE COUNTER (H)</td><td>—</td></tr><tr><td>$06</td><td>R/W</td><td colspan="8">TIMER 2 (L)</td><td>—</td></tr><tr><td>$07</td><td>R/W</td><td colspan="8">TIMER 2 (H)</td><td>—</td></tr><tr><td>$08</td><td>W</td><td>*1</td><td>T2MSK</td><td>T1MSK</td><td>T0MSK</td><td>STBC</td><td>ST2</td><td>ST1</td><td>ST0</td><td>—</td></tr><tr><td>$09</td><td>W</td><td>ADPRST</td><td>R</td><td>L</td><td>FS1</td><td>FS0</td><td>PCM</td><td>PLY/REC</td><td>ADPST</td><td>W</td><td>ADPRST</td><td>R</td><td>L</td><td>FS1</td><td>FS0</td><td>PCM</td><td>PLY/REC</td><td>ADPST</td></tr><tr><td>$0A</td><td>W</td><td colspan="8">VOLUME CONTROL</td><td>W</td><td colspan="8">VOLUME CONTROL</td></tr><tr><td>$0B</td><td>R/W</td><td colspan="8">PCM DATA</td><td>R/W</td><td colspan="8">PCM DATA</td></tr><tr><td>$0C</td><td>W</td><td>DMA MOD</td><td>FMT1</td><td>FMT0</td><td>SELF2</td><td>SELF1</td><td>SELF0</td><td>MSK FIF</td><td>DMA ENB</td><td>W</td><td></td><td>FMT1</td><td>FMT0</td><td>SELF2</td><td>SELF1</td><td>SELF0</td><td>MSK FIF</td><td>DMA ENB</td></tr><tr><td>$0D</td><td>W</td><td colspan="2"></td><td>MSKPOV</td><td>MSKMOV</td><td>MDITRSRST</td><td>MSKTRQ</td><td>MDIRCVRST</td><td>MSKRRQ</td><td>W</td><td colspan="2"></td><td>MSKPOV</td><td>MSKMOV</td><td>MDITRSRST</td><td>MSKTRQ</td><td>MDIRCVRST</td><td>MSKRRQ</td></tr><tr><td>$0E</td><td>R/W</td><td colspan="8">MIDI DATA</td><td>R/W</td><td colspan="8">MIDI DATA</td></tr></table>

注) 斜線部は don't care、*1 は必ず '0' として下さい。レジスタ値はイニシャルクリアで、SELT、ADPRST、MDITRSRST、MDIRCVRST 以外は '0' となります。

### 2. レジスタ説明

PCM/ADPCM の CHANNEL 1、2 は A1 端子によって選択します。R/W 欄に R/W と記されているレジスタはデータリードモードによって読み出し可能です。

<table border="1"><tr><td>アドレス</td><td>名称</td><td>機能</td></tr><tr><td>$00</td><td>SELT</td><td>PCM データタイプ (2's コンプリメントまたはオフセットバイナリ) を選択します。</td></tr><tr><td>$01</td><td>LSI TEST</td><td>本 LSI のテストに使用されます。</td></tr><tr><td>$02〜03</td><td>TIMER 0</td><td>16 ビットのプログラマブルダウンカウンタです。</td></tr><tr><td>$04〜05</td><td>BASE COUNTER</td><td>タイマー 1、タイマー 2 へのクロックを供給する 12 ビットプログラマブルダウンカウンタです。</td></tr><tr><td>$05</td><td>TIMER 1</td><td>ベースカウンタのクロックで動作する 4 ビットプログラマブルダウンカウンタです。</td></tr><tr><td>$06〜07</td><td>TIMER 2</td><td>ベースカウンタのクロックで動作する 16 ビットプログラマブルダウンカウンタです。</td></tr><tr><td>$08</td><td>T0MSK, T1MSK, T2MSK</td><td>タイマー 0、タイマー 1、タイマー 2 から発生する IRQ 信号のみをマスクします。ステータスレジスタのフラグはマスクされません。</td></tr><tr><td>$08</td><td>ST0, ST1, ST2, STBC</td><td>タイマー 0、タイマー 1、タイマー 2 及びベースカウンタの始動・停止を制御します。</td></tr><tr><td>$09</td><td>ADPRST</td><td>PCM/ADPCM 部をリセットします。</td></tr><tr><td>$09</td><td>L, R</td><td>出力するチャンネルを選択します。</td></tr><tr><td>$09</td><td>FS0, FS1</td><td>PCM/ADPCM のサンプリング周波数を選択します。</td></tr><tr><td>$09</td><td>PCM</td><td>PCM モード・ADPCM モードを選択します。</td></tr><tr><td>$09</td><td>PLY/REC</td><td>録音・再生を選択します。</td></tr><tr><td>$09</td><td>ADPST</td><td>録音・再生の始動・停止を制御します。</td></tr><tr><td>$0A</td><td>VOLUME CONTROL</td><td>出力ボリューム値を設定します。</td></tr><tr><td>$0B</td><td>PCM DATA</td><td>FIFO へのデータの書き込み、FIFO からのデータの読み出しを行います。</td></tr><tr><td>$0C</td><td>DMAMOD</td><td>DMA コントローラを 1 チャンネル使用して CHANNEL 1、2 のデータを交互に転送する 1 チャンネル DMA モードを選択します。</td></tr><tr><td>$0C</td><td>FMT0, FMT1</td><td>PCM データフォーマットを選択します。</td></tr><tr><td>$0C</td><td>SELF0, SELF1, SELF2</td><td>FIFO 割り込み発生ポイントを選択します。</td></tr><tr><td>$0C</td><td>MSKFIF</td><td>FIFO 割り込み信号によって発生する IRQ 信号のみをマスクします。ステータスレジスタのフラグはマスクされません。</td></tr><tr><td>$0C</td><td>DMAENB</td><td>DMA モード / CPU モードを選択します。</td></tr><tr><td>$0D</td><td>MSKPOV</td><td>PCM/ADPCM 録音時のオーバーランエラーによる IRQ 信号のみをマスクします。ステータスレジスタのフラグはマスクされません。</td></tr><tr><td>$0D</td><td>MSKMOV</td><td>MIDI 受信時のオーバーランエラーによる IRQ 信号のみをマスクします。ステータスレジスタのフラグはマスクされません。</td></tr><tr><td>$0D</td><td>MDITRSRST</td><td>MIDI 送信回路をリセットします。</td></tr><tr><td>$0D</td><td>MSKTRQ</td><td>MIDI 送信用 FIFO の IRQ 信号のみをマスクします。ステータスレジスタのフラグはマスクされません。</td></tr><tr><td>$0D</td><td>MDIRCVRST</td><td>MIDI 受信回路をリセットします。</td></tr><tr><td>$0D</td><td>MSKRRQ</td><td>MIDI 受信用 FIFO の IRQ 信号のみをマスクします。ステータスレジスタのフラグはマスクされません。</td></tr><tr><td>$0E</td><td>MIDIDATA</td><td>MIDI の FIFO へのデータ書き込み、FIFO からのデータの読み出しを行います。</td></tr></table>

### 3. ステータスアサイン

<table border="1"><tr><td>ビット</td><td>D7</td><td>D6</td><td>D5</td><td>D4</td><td>D3</td><td>D2</td><td>D1</td><td>D0</td></tr><tr><td>ステータス</td><td>OV</td><td>T2</td><td>T1</td><td>T0</td><td>TRQ</td><td>RRQ</td><td>FIF2</td><td>FIF1</td></tr></table>

### 4. ステータス説明

以下に示す各ブロックからの割り込み信号が発生するとそれぞれに対応するステータスレジスタのビットは '1' となり、同時に /IRQ 端子を 'L' にして CPU へ通知します。

ただし、/IRQ = 'L' として CPU へ通知するのは、それぞれの割り込みに対応したマスクビット (T0MSK, T1MSK, T2MSK, MSKFIF, MSKPOV, MSKMOV, MSKTRQ, MSKRRQ) が '0' の場合です。

<table border="1"><tr><td>名称</td><td>機能</td></tr><tr><td>OV</td><td>MIDI 受信時または PCM/ADPCM 録音時及び再生時のオーバーランエラーで '1' となります。</td></tr><tr><td>T0, T1, T2</td><td>各々のタイマーのカウンタ値が 0 になると '1' となります。</td></tr><tr><td>TRQ</td><td>MIDI 送信 FIFO が空になると '1' となります。</td></tr><tr><td>RRQ</td><td>MIDI 受信 FIFO にデータがセットされると '1' となります。</td></tr><tr><td>FIF1, FIF2</td><td>PCM/ADPCM の FIFO のデータ量が SELF2, 1, 0 で設定したポイントになると '1' となります。</td></tr></table>

## 電気的特性

### 1. 絶対最大定格

<table border="1"><tr><td>項目</td><td>記号</td><td>定格値</td><td>単位</td></tr><tr><td>電源電圧</td><td>$V_{DD}$</td><td>$-0.3 \sim 7.0$</td><td>V</td></tr><tr><td>入力電圧</td><td>$V_{I}$</td><td>$-0.3 \sim V_{DD}+0.5$</td><td>V</td></tr><tr><td>動作温度</td><td>$T_{op}$</td><td>$0 \sim 70$</td><td>℃</td></tr><tr><td>保存温度</td><td>$T_{stg}$</td><td>$-50 \sim 125$</td><td>℃</td></tr></table>

### 2. 推奨動作条件

<table border="1"><tr><td>項目</td><td>記号</td><td>最小</td><td>標準</td><td>最大</td><td>単位</td></tr><tr><td>電源電圧</td><td>$V_{DD}$</td><td>4.75</td><td>5.00</td><td>5.25</td><td>V</td></tr><tr><td>動作温度</td><td>$T_{op}$</td><td>0</td><td>25</td><td>70</td><td>℃</td></tr></table>

<div align="center">

### 3. 直流特性

(条件: $T_{a} = 0 \sim 70^{\circ}\mathrm{C}$, $V_{DD} = 5.0 \pm 0.25\mathrm{V}$)

</div>

<table border="1"><tr><td>項目</td><td>記号</td><td>条件</td><td>最小</td><td>標準</td><td>最大</td><td>単位</td></tr><tr><td>消費電力</td><td>$P_d$</td><td>$V_{DD}=5.0\mathrm{V}$, $f_M=16.9344\mathrm{MHz}$</td><td></td><td></td><td>200</td><td>mW</td></tr><tr><td>入力電圧 H レベル (1)</td><td>$V_{IH1}$</td><td>*1</td><td>2.2</td><td></td><td></td><td>V</td></tr><tr><td>入力電圧 L レベル (1)</td><td>$V_{IL1}$</td><td>*1</td><td></td><td></td><td>0.8</td><td>V</td></tr><tr><td>入力電圧 H レベル (2)</td><td>$V_{IH2}$</td><td>*2</td><td>3.5</td><td></td><td></td><td>V</td></tr><tr><td>入力電圧 L レベル (2)</td><td>$V_{IL2}$</td><td>*2</td><td></td><td></td><td>1.0</td><td>V</td></tr><tr><td>入力リーク電流</td><td>$I_{L1}$</td><td>$V_I = 0 \sim 5\mathrm{V}$, *3</td><td>-10</td><td></td><td>10</td><td>μA</td></tr><tr><td>入力容量</td><td>$C_I$</td><td></td><td></td><td></td><td>10</td><td>pF</td></tr><tr><td>出力電圧 H レベル</td><td>$V_{OH}$</td><td>$I_{OH} = -80\mu\mathrm{A}$</td><td>$V_{DD}-1.0$</td><td></td><td></td><td>V</td></tr><tr><td>出力電圧 L レベル</td><td>$V_{OL}$</td><td>$I_{OL} = 2.0\mathrm{mA}$</td><td></td><td></td><td>$V_{SS}+0.4$</td><td>V</td></tr><tr><td>出力容量</td><td>$C_O$</td><td></td><td></td><td></td><td>10</td><td>pF</td></tr><tr><td>出力リーク電流</td><td>$I_{L0}$</td><td>$V_I = 0 \sim 5\mathrm{V}$, *4</td><td>-10</td><td></td><td>10</td><td>μA</td></tr><tr><td>プルアップ抵抗</td><td>$R_U$</td><td></td><td>80</td><td></td><td>400</td><td>kΩ</td></tr></table>

注)

- *1 : /WR, /RD, /CS, A0〜A9, AEN, D0〜D7, RXD, /CSGP, GP4〜GP7, /DACK1, /DACK2 に適用。(ただし D0〜D7 は入力状態の時に適用)
- *2 : XI, /IC に適用
- *3 : /WR, /RD, A0〜A9, AEN, D0〜D7, RXD, /CSGP, GP4〜GP7 に通用。(ただし D0〜D7 は入力状態の時に適用)
- *4 : D0〜D7 において、ハイインピーダンス状態時

<div align="center">

### 4. 交流特性

(条件: $T_{a} = 0 \sim 70^{\circ}\mathrm{C}$, $V_{DD} = 5.0 \pm 0.25\mathrm{V}$)

</div>

<table border="1"><tr><td>項目</td><td>記号</td><td>図</td><td>最小</td><td>標準</td><td>最大</td><td>単位</td></tr><tr><td>マスタークロック周波数</td><td>$f_M$</td><td>A-1</td><td></td><td>16.9344</td><td></td><td>MHz</td></tr><tr><td>デューティ</td><td>$D$</td><td></td><td>45</td><td>50</td><td>55</td><td>%</td></tr><tr><td>リセットパルス幅</td><td>$t_{ICW}$</td><td>A-2</td><td>80</td><td></td><td></td><td>サイクル *1</td></tr><tr><td>アドレスセットアップ時間</td><td>$t_{AS}$</td><td>A-3, 4</td><td>10</td><td></td><td></td><td>ns</td></tr><tr><td>アドレスホールド時間</td><td>$t_{AH}$</td><td>A-3, 4</td><td>10</td><td></td><td></td><td>ns</td></tr><tr><td>チップセレクトライト幅</td><td>$t_{CSW}$</td><td>A-3</td><td>50</td><td></td><td></td><td>ns</td></tr><tr><td>チップセレクトリード幅</td><td>$t_{CSR}$</td><td>A-4</td><td>100</td><td></td><td></td><td>ns</td></tr><tr><td>ライトパルス幅</td><td>$t_{WW}$</td><td>A-3</td><td>50</td><td></td><td></td><td>ns</td></tr><tr><td>ライトデータセットアップ時間</td><td>$t_{WDS}$</td><td>A-3</td><td>10</td><td></td><td></td><td>ns</td></tr><tr><td>ライトデータホールド時間</td><td>$t_{WDH}$</td><td>A-3</td><td>20</td><td></td><td></td><td>ns</td></tr><tr><td>リードパルス幅</td><td>$t_{RW}$</td><td>A-4</td><td>100</td><td></td><td></td><td>ns</td></tr><tr><td>リードデータアクセス時間</td><td>$t_{ACC}$</td><td>A-4</td><td></td><td></td><td>100</td><td>ns</td></tr><tr><td>リードデータホールド時間</td><td>$t_{RDH}$</td><td>A-4</td><td>10</td><td></td><td></td><td>ns</td></tr><tr><td>DRQ ホールド時間</td><td>$t_{DRQH}$</td><td>A-5</td><td></td><td></td><td>50</td><td>ns</td></tr><tr><td>DMA リードセットアップ時間</td><td>$t_{DRS}$</td><td>A-5</td><td>50</td><td></td><td></td><td>ns</td></tr><tr><td>DMA リードホールド時間</td><td>$t_{DRH}$</td><td>A-5</td><td>20</td><td></td><td></td><td>ns</td></tr><tr><td>DMA リードデータアクセス時間</td><td>$t_{DRAC}$</td><td>A-5</td><td></td><td></td><td>100</td><td>ns</td></tr><tr><td>DMA リードデータホールド時間</td><td>$t_{DRDH}$</td><td>A-5</td><td>10</td><td></td><td></td><td>ns</td></tr><tr><td>DMA ライトセットアップ時間</td><td>$t_{DWS}$</td><td>A-6</td><td>50</td><td></td><td></td><td>ns</td></tr><tr><td>DMA ライトホールド時間</td><td>$t_{DWH}$</td><td>A-6</td><td>20</td><td></td><td></td><td>ns</td></tr></table>

注) *1 : マスタークロックのサイクルにおいて

<div align="center">

### 5. アナログ特性

(条件: $T_{a} = 0 \sim 70^{\circ}\mathrm{C}$, $AV_{DD} = 5.0\mathrm{V}$)

</div>

<table border="1"><tr><td>項目</td><td>記号</td><td>条件</td><td>最小</td><td>標準</td><td>最大</td><td>単位</td></tr><tr><td>アナログ入力電圧</td><td>$V_{IA}$</td><td>*1</td><td></td><td></td><td>4.8</td><td>V</td></tr><tr><td>アナログ出力電圧</td><td>$V_{OA}$</td><td>*2</td><td></td><td></td><td>4.8</td><td>V</td></tr><tr><td>DC オフセット電圧</td><td>$CV$</td><td>*3</td><td></td><td>2.5</td><td></td><td>V</td></tr><tr><td>オフセット電圧</td><td>$V_{OFF}$</td><td>*2</td><td></td><td></td><td>0.1</td><td>V</td></tr><tr><td>リニアリティ誤差</td><td></td><td>*2</td><td></td><td></td><td>±30</td><td>mV</td></tr><tr><td>ステップ誤差</td><td></td><td>*2</td><td></td><td></td><td>±1.0</td><td>LSB</td></tr></table>

注)

- *1 : AIN1, AIN2 に適用
- *2 : L1, R1, L2, R2 に適用
- *3 : CV に適用

### 6. タイミング図

**(1) 入力クロックタイミング**

![図 A-1 入力クロックタイミング](images/timing-a1-input-clock.png)

**(2) リセットパルス**

![図 A-2 リセットパルス](images/timing-a2-reset.png)

**(3) アドレス、及びデータライトタイミング**

![図 A-3 アドレス・データライトタイミング](images/timing-a3-write.png)

$t_{CSW}$, $t_{WW}$, $t_{WDH}$ は /CS, /WR のいずれかが High レベルになった時を基準とする。

**(4) ステータス、及びデータリードタイミング**

![図 A-4 ステータス・データリードタイミング](images/timing-a4-read.png)

$t_{ACC}$ は /CS, /RD の遅く Low レベルになるのが基準です。

$t_{CSR}$, $t_{RW}$, $t_{RDH}$ は /CS, /RD のいずれかが High レベルになった時を基準とする。

**(5) DMA リードタイミング**

![図 A-5 DMA リードタイミング](images/timing-a5-dma-read.png)

**(6) DMA ライトタイミング**

![図 A-6 DMA ライトタイミング](images/timing-a6-dma-write.png)

## パッケージ外形図

**YMZ263B-F**

![パッケージ外形図 (1)](images/package-1.png)

![パッケージ外形図 (2)](images/package-2.png)
