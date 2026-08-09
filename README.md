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
- 大明槓・加槓後に嶺上和了が可能な場合、`decide` は最初に `TSUMO` / `PASS`、
  `PASS` 後に槓ドラ表示牌を公開したviewと打牌・連続槓候補で再度呼ばれる

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
- 公開情報（捨て牌・副露・ドラ表示牌）
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
- 九種九牌 / 四風連打 / 四家立直 / 四槓散了
- 流し満貫
- 責任払い（大三元・大四喜・四槓子）

### v1.1.x 対応ルール

- 立直宣言は「宣言中」と「成立済み」を分離
  - 宣言牌へのロンがなければ、鳴かれた場合も立直成立
  - 成立時に1000点減算、供託追加、一発開始
  - 四家立直は成立済み立直のみを数える
- 喰い替え禁止
  - ポン/チー直後の1打だけ、同種牌と両面チー外側の筋喰い替えを禁止可能
- 槓ドラ表示タイミング
  - 暗槓: 暗槓 → 槓ドラ表示 → 嶺上ツモ
  - 大明槓/加槓: 槓 → 嶺上ツモ → 槓ドラ表示 → 打牌
  - 大明槓/加槓の新しい槓ドラは、その槓の嶺上開花には適用しない
  - 打牌せず連続して槓する場合は、次の嶺上ツモ前に表示する
- 国士無双に限る暗槓槍槓
- 三家和の途中流局切り替え
- 特殊形ダブル役満、数え役満、切り上げ満貫の切り替え
- 複合役満時の責任払い範囲切り替え
- ルール生成/検証API
  - `cj4_rules_default()`
  - `cj4_rules_validate()`
  - `cj4_rules_tenhou()`
  - `cj4_rules_mjsoul()`

### ルールフラグ / Rule flags

| フィールド | 既定値 | 意味 |
| --- | ---: | --- |
| `kuitan` | 1 | 喰い断を有効化 |
| `kuikae_forbidden` | 1 | 鳴き直後1打の喰い替え禁止 |
| `ippatsu` | 1 | 一発役を有効化 |
| `max_ron_players` | 3 | 同一打牌へのロン最大人数（1=頭ハネ、2=二家和、3=三家和まで） |
| `kokushi_ron_on_ankan` | 1 | 国士無双に限り暗槓へのロンを許可 |
| `triple_ron_abortive_draw` | 0 | 3人ロンを三家和流局にする |
| `noten_penalty` | 1 | 流局時ノーテン罰符 |
| `noten_penalty_points` | 3000 | ノーテン罰符の総額 |
| `abortive_kyuushu_kyuuhai` | 1 | 九種九牌 |
| `abortive_suufon_renda` | 1 | 四風連打 |
| `abortive_four_riichi` | 1 | 四家立直 |
| `nagashi_mangan` | 1 | 流し満貫 |
| `kokushi_13_wait_double` | 1 | 国士無双十三面待ちをダブル役満にする |
| `suuankou_tanki_double` | 1 | 四暗刻単騎をダブル役満にする |
| `junsei_chuuren_double` | 1 | 純正九蓮宝燈をダブル役満にする |
| `daisuushii_double` | 1 | 大四喜をダブル役満にする |
| `kazoe_yakuman` | 1 | 13翻以上を数え役満にする |
| `kiriage_mangan` | 1 | 30符4翻/60符3翻を満貫に切り上げる |
| `pao` | 1 | 責任払いを有効化 |
| `pao_liability_only` | 0 | 1なら責任役満部分だけを責任払い対象にする |
| `pao_daisangen` | 1 | 大三元の責任払いを有効化 |
| `pao_daisuushii` | 1 | 大四喜の責任払いを有効化 |
| `pao_suukantsu` | 1 | 四槓子の責任払いを有効化 |
| `multi_ron_honba_first_only` | 0 | 複数ロン時の本場を放銃者から見て最初の和了者だけに付与 |
| `nagashi_dealer_tenpai_renchan` | 0 | 流し満貫時の連荘を親の聴牌状態で判定 |
| `target_score_excludes_riichi_sticks` | 0 | 目標点とトップ判定から、その局で獲得した供託点を除外 |
| `aka_tiles` | 5m/5p/5s 各1枚 | 赤牌IDを指定 |

`cj4_rules_default()` は一般的な4人打ちリーチ麻雀としてそのまま対局できる値を返し、`version = CJ4_RULES_VERSION` を設定します。`{0}` 初期化された `cj4_rules` は v1.0 互換ルールとして扱われ、国士無双十三面待ち・四暗刻単騎・純正九蓮宝燈・大四喜のダブル役満、および13翻以上の数え役満は従来どおり有効です。新規コードでこれらを明示的に無効化する場合は、`cj4_rules_default()` などのプリセットから該当フラグを0にしてください。

### プリセット差分

| プリセット | 差分 |
| --- | --- |
| `cj4_rules_default()` | 標準設定。三家和は流局せず3人ロン、責任払いは和了全体対象 |
| `cj4_rules_tenhou()` | 三家和流局、本場・供託の上家取り、流し満貫時の親聴牌連荘、供託を除く終局判定を有効化。切り上げ満貫、暗槓国士槍槓、特殊形ダブル役満を無効化。責任払いは大三元・大四喜のみ、複合役満を含む和了全体対象 |
| `cj4_rules_mjsoul()` | 現時点では default と同じく三家和は3人ロン、責任払いは和了全体対象 |

非対応：

- 三人麻雀（別実装想定）
- 人和、途中流局の詳細差分、ローカル役
- 供託・順位点を含む最終精算
- 複合役満で役満ごとに責任者が異なる責任払い

---

## 互換性 / Compatibility

v1.1.0 は公開構造体 `cj4_rules` / `cj4_mahjong` を拡張します。ソース互換を基準とし、v1.0.0 とのバイナリABI互換は保証しません。

v1.1.1 は公開構造体 `cj4_player_view` を拡張します。v1.1.0 とのバイナリABI互換は保証しません。

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

1.1.1 リリース<br>
1.1.1 release

---

## ライセンス / License

MIT License
