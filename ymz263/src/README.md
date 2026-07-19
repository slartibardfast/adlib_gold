# YMZ263B (MMA) — 概要

[English →](./README.en.md)

本書はヤマハ株式会社製 **YMZ263B** (Multimedia Audio & Game Interface Controller、通称 **MMA**) のデータシートの検索可能 mdBook 版です。オリジナルの日本語 PDF を GLM-OCR により文字化し、OCR 誤認識を修復した上で、元の構成・レジスタ表・電気的特性・タイミング図を再現しています。

## 関連文書との関係

- Ad Lib Gold Developer Toolkit マニュアルの [Chapter 7 (Low-Level Programming)](https://slartibardfast.github.io/adlib_gold/ch07-low-level.html) は本チップの MMA 部を抜粋・英訳した二次資料です。本書は一次資料 (ヤマハ原本) にあたります。
- Ad Lib Gold WDM ドライバ (`adlibgold.sys`) のレジスタプログラミングは本データシートを権威ある仕様として参照します。

## 提供内容

- [データシート本文 (日本語)](./datasheet.md) — オリジナルの日本語を清書した版。
- [Datasheet (English translation)](./datasheet.en.md) — 日本語の横に並べる英語訳。技術用語 (端子名、レジスタ名、信号名) は原本の表記を保持しています。

## 図の一覧

| 図 | 内容 | ファイル |
|---|---|---|
| ブロック図 | チップ内部構成 | `images/block-diagram.png` |
| 端子配置図 | 64 ピン QFP | `images/pin-layout.png` |
| GP0〜7 入力等価回路 | ゲームポート | `images/game-port-circuit.png` |
| 図 A-1 | 入力クロックタイミング | `images/timing-a1-input-clock.png` |
| 図 A-2 | リセットパルス | `images/timing-a2-reset.png` |
| 図 A-3 | アドレス・データライトタイミング | `images/timing-a3-write.png` |
| 図 A-4 | ステータス・データリードタイミング | `images/timing-a4-read.png` |
| 図 A-5 | DMA リードタイミング | `images/timing-a5-dma-read.png` |
| 図 A-6 | DMA ライトタイミング | `images/timing-a6-dma-write.png` |
| パッケージ外形図 (1) | 64 ピン QFP 寸法 | `images/package-1.png` |
| パッケージ外形図 (2) | 64 ピン QFP 寸法 | `images/package-2.png` |

## 出典

- オリジナル PDF: [theretroweb.com — ymz263b-67b9e7aead79f115900527.pdf](https://theretroweb.com/chip/documentation/ymz263b-67b9e7aead79f115900527.pdf)
- OCR: GLM-OCR (2026-07-19)
- 転載にあたりヤマハ株式会社の文書を原本とし、技術内容の改変は行っていません。OCR 誤認識の修復と mdBook 形式への再構成のみを行っています。
