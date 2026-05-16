# cjong4

4人打ち日本式麻雀（リーチ麻雀）のための、純粋関数型 C ライブラリ。  
A pure functional C library for 4-player Japanese mahjong (riichi mahjong).

---

## 概要 / Overview

**cjong4** は、4人麻雀のコアロジックを純粋関数に基づいて実装する C ライブラリです。  
**cjong4** is a C library that implements the core logic of 4-player mahjong using pure functions.

本プロジェクトは以下の原則に基づいて設計されます：  
This project is designed with the following principles:

- グローバルな可変状態を持たない  
  No mutable global state
- すべての処理は純粋関数として実装する  
  All operations are implemented as pure functions
- 状態は完全に値として表現する  
  State is fully represented as values
- 振る舞いは決定的で再現可能である  
  Behavior is deterministic and reproducible

---

## 設計思想 / Design Philosophy

### 位置ベースアーキテクチャ / Position-Based Architecture

一般的な麻雀実装では手牌は集合や枚数として扱われますが、  
**cjong4 では位置に基づくモデルを採用します。**

Typical mahjong implementations treat hands as sets or tile counts.  
**cjong4 adopts a position-based model.**

- すべての牌は固定された位置を持つ  
  Every tile has a fixed position
- 状態は配置として表現される  
  State is represented as a layout
- ソートを必要としない  
  No sorting is required
- 同一性は構造的に判定できる  
  Equality is structural and deterministic

---

### 純粋関数 / Pure Functions

すべてのロジックは純粋関数として実装されます。  
All logic is implemented as pure functions.

- 副作用を持たない  
  No side effects
- 隠れた状態を持たない  
  No hidden state
- 同じ入力は常に同じ出力を返す  
  Same input always produces the same output

---

### 4人打ち特化 / 4-Player Specific

本ライブラリは 4人打ち麻雀専用です。  
This library is strictly for 4-player mahjong.

- プレイヤー数は常に4  
  Fixed number of players: 4
- 順序は固定  
  Fixed turn order
- 手牌枚数は固定（13/14枚）  
  Fixed hand size (13/14 tiles)

対応ルール / Supported formats:

- 東風戦 / East-only game
- 半荘 / Half game

3人麻雀は対象外とし、別ライブラリとして実装する前提です。  
3-player mahjong (sanma) is not supported and should be implemented separately.

---

## 対応規格 / Language Standard

本ライブラリは以下のC言語規格を対象とします：  
This library targets the following C standard:

- ISO C11

GCC / Clang / MSVC でのビルドを想定しています。  
Tested with GCC, Clang, and MSVC.

---

## 実装予定 / Planned Features

- 牌の表現 / Tile representation
- 手牌操作 / Hand operations
- 副露（チー・ポン・カン） / Melds (chi, pon, kan)
- 和了判定 / Win detection
- 点数計算 / Score calculation
- 局進行 / Round progression
- ゲーム進行 / Game progression

---

## 命名規則 / Naming Convention

公開APIは `cj4_` プレフィックスを使用します。  
All public APIs use the `cj4_` prefix.

例 / Examples:

cj4_state cj4_tile cj4_apply_action cj4_is_agari

---

## ディレクトリ構成（予定） / Project Structure (Planned)

include/ cjong4/
src/

---

## ステータス / Status

開発中 / Work in progress

---

## ライセンス / License

MIT License