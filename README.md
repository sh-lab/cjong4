# cjong4

4人打ち日本式麻雀（リーチ麻雀）のための C ライブラリ。  
Pure functional-style C library for 4-player Japanese mahjong (riichi mahjong).

---

## 概要 / Overview

**cjong4** は、4人麻雀のコアロジックを純粋関数的な設計で実装した C ライブラリです。  
**cjong4** is a C library implementing the core logic of 4-player mahjong with a functionally-oriented design.

このライブラリは以下の原則に基づいて設計されています：  
This library is designed with the following principles:

- グローバルな可変状態を持たない  
  No mutable global state
- 状態は値として扱う  
  State is treated as immutable values
- 処理は決定的かつ再現可能  
  Deterministic and reproducible behavior
- 副作用を最小限に抑える  
  Minimized side effects

---

## アーキテクチャ / Architecture

cjong4 は以下の3層構造で設計されています：

### core

- ゲーム状態（state）
- 行動（action）
- 判定（can_*）
- 状態遷移（do_*）

完全情報・純粋関数で構成される。

---

### manager

- 行動候補の収集
- プレイヤー意思決定（delegate）
- 優先順位解決（ロン・ポン・チー）
- ゲーム進行制御

---

### player interaction

プレイヤーは `cj4m_player_delegate` として実装される：

cj4_action (*decide)(
    void *ctx,
    const cj4_player_view *view,
    const cj4_action *actions,
    uint8_t action_count)

- `view` は可視情報のみを含む
- `actions` は合法手一覧
- プレイヤーは1つ選択する
- `ctx` と `decide` を `cj4m_player_delegate` に束ねて `cj4m_step` に渡す

---

## 設計思想 / Design Philosophy

### 位置ベースアーキテクチャ / Position-Based Model

cjong4 は牌を「集合」ではなく「位置」で管理します。

- 各牌は固定IDを持つ
- 状態は配置で表現される
- ソート不要
- 同一性は構造的に比較可能

---

### 純粋関数指向 / Functional Approach

- 関数は入力から出力のみを生成する
- 隠れた状態を持たない
- 同じ入力は常に同じ結果

---

### 情報非対称 / Partial Information

プレイヤーには `cj4_player_view` を通じて可視情報のみ提供される。

- 自分の手牌
- 公開情報（捨て牌・副露）
- その他プレイヤー状態（リーチなど）

非公開情報（他人の手牌・山）は含まれない。

---

## ゲームループ / Game Loop

ライブラリは外部駆動型です：

```c
while (state.phase != CJ4_PHASE_GAME_END)
{
    state = cj4m_step(&state, &rules, delegates);

    if (cj4_can_next_round(state))
    {
        cj4_tile_id next_wall[CJ4_TILE_ID_COUNT];
        fill_next_wall(next_wall);
        state = cj4_do_next_round(state, next_wall, &rules);
    }
}
```

- `cj4m_step` は局内の進行を1ステップ進める
- 次局開始時の wall 供給は呼び出し側が行う
- UI・AI・ログと容易に統合可能

---

## 対応仕様 / Scope

- 4人打ち固定
- 東風戦 / 半荘
- 日本式リーチ麻雀

非対応：

- 三人麻雀（別実装想定）

---

## C言語仕様 / Language Standard

- ISO C11

対応コンパイラ：

- GCC
- Clang
- MSVC

---

## 命名規則 / Naming Convention

- core API: `cj4_*`
- manager API: `cj4m_*`

例:

cj4_do_discard cj4_can_ron cj4m_step cj4m_collect_actions

---

## ディレクトリ構成 / Project Structure

```
include/cjong4/core/      core public API
include/cjong4/manager/   manager public API
src/core/                 core implementation 
src/manager/              manager implementation
tests/core/               core tests 
tests/manager/            manager tests
```

---

## ステータス / Status

開発中（v1 機能は概ね完成）  
Work in progress (core functionality mostly complete)

---

## ライセンス / License

MIT License