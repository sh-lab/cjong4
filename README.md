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

## ビルド / Build

必要なもの：

- CMake 3.16以降
- ISO C11対応コンパイラ（GCC、Clang、MSVC）

Releaseビルド：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

## テスト / Test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## インストール / Install

任意のprefixへ静的ライブラリ、公開ヘッダ、CMake package filesをインストールできます。

```sh
cmake --install build --config Release --prefix /path/to/prefix
```

CMakeプロジェクトから利用する場合：

```cmake
find_package(cjong4 3 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE cjong4::cj4)
```

標準の探索先以外へインストールした場合は、利用側の構成時にprefixを指定します。

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
```

---

## アーキテクチャ / Architecture

cjong4 は以下の3層構造で設計されています：

詳細な構成、状態モデル、依存方向、不変条件は
[アーキテクチャ概要](doc/architecture.md)を参照してください。重要な設計判断と
その背景は[ADR一覧](doc/adr/README.md)に記録しています。

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
- 嶺上牌ツモ後も、通常のツモ後と同様にツモ和了・打牌・連続槓候補が一度に渡される

### 手牌解析 / Hand analysis

完全状態から解析する場合は `cjong4/core/hand_analysis.h` の `cj4_*` APIを、
プレイヤーデリゲートから解析する場合は `cjong4/player/hand_analysis.h` の
`cj4p_*` APIを使用します。プレイヤー実装が `manager.h` を読み込む必要はありません。

- `cj4_shanten_result` は通常形、七対子、国士無双のシャンテン数を個別に返す
- 13枚相当は現在の手牌、14枚相当は任意の1枚を切った後の最小値を各形式別に返す
- 副露時の七対子と国士無双は `CJ4_SHANTEN_NOT_APPLICABLE`
- 待ち判定は形テンとし、役と振り聴は考慮しない
- 待ちの `count[type]` は、自分の手牌、全員の捨牌・面子、公開済みドラ表示牌を除いた見えていない枚数
- `cj4_live_wall_remaining()` と `cj4_player_view.live_wall_remaining` は、嶺上牌を除く通常ツモの残数を返す

---

## 設計思想 / Design Philosophy

### 位置ベースアーキテクチャ / Position-Based Model

cjong4 は牌を「集合」ではなく「位置」で管理します。

- 各牌は固定IDを持つ
- `locations[136]` を牌配置の唯一の正規データとする
- 各牌の4バイトに元山位置・捨牌・手牌/鳴牌・捨牌履歴を保持する
- 山・捨牌・手牌・鳴牌は必要時に最大136枚を走査して復元する
- ツモ牌と最新捨牌だけは牌IDを直接保持する
- ソート不要
- 同一性は構造的に比較可能

`cj4_location` は常に4バイトで、各バイトの未使用値は `0xFF` です。1牌の位置は
`cj4_location_get()` で取得します。手牌・捨牌・鳴牌・ドラ表示牌は
`cj4_location_collect_*()` が返すリスト値として復元できます。完全状態には
`cj4_mahjong.locations`、プレイヤー別のマスク済み情報には
`cj4_player_view.locations` を渡します。

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
while (cj4_state_phase(&state) != CJ4_PHASE_GAME_END)
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
- wall は `0`〜`135` の物理牌IDを各1回含む必要があり、
  `cj4_wall_is_valid()` で事前検証できる
- UI・AI・ログと容易に統合可能

---

## 対応仕様 / Scope

- 4人打ち固定
- 東風戦 / 半荘
- 日本式リーチ麻雀
- 九種九牌 / 四風連打 / 四家立直 / 四槓散了
- 流し満貫
- 責任払い（大三元・大四喜・四槓子）

### v3 対応ルール

- 立直宣言は「宣言中」と「成立済み」を分離
  - 宣言牌へのロンがなければ、鳴かれた場合も立直成立
  - 成立時に1000点減算、供託追加、一発開始
  - 四家立直は成立済み立直のみを数える
- 喰い替え禁止
  - ポン/チー直後の1打だけ、同種牌と両面チー外側の筋喰い替えを禁止可能
- 槓ドラ表示タイミング
  - 暗槓は常に成立時に表示
  - 大明槓・加槓は先めくり／後めくりを選択可能
  - 後めくりでは槓後の打牌記録後、ロン・ポン・チー判定前に表示
  - 打牌せず連続して槓する場合は、次の槍槓判定前に前の槓ドラを表示
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
| `kan_dora_timing` | `CJ4_KAN_DORA_EARLY` | 大明槓・加槓の槓ドラ表示タイミング |
| `four_kans_abort_timing` | `CJ4_FOUR_KANS_ABORT_AFTER_DISCARD` | 複数人による4槓時に即時流局するか、4槓目の嶺上牌を打牌してロンがなかった後に流局するかを選択 |
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

`four_kans_abort_timing` が `CJ4_FOUR_KANS_ABORT_AFTER_DISCARD` の場合、4槓目の嶺上牌によるツモ和了と、その後の打牌へのロン和了が優先されます。ロンがなければ四槓散了となり、チー・ポン・大明槓はできません。四槓子成立後に行われる5回目の槓宣言は、設定にかかわらず宣言時に四槓散了となります。

### プリセット差分

| プリセット | 差分 |
| --- | --- |
| `cj4_rules_default()` | 標準設定。4槓目は嶺上牌の打牌後に流局。槓ドラは先めくり、三家和は流局せず3人ロン、責任払いは和了全体対象 |
| `cj4_rules_tenhou()` | 4槓目は嶺上牌の打牌後に流局。槓ドラは後めくり。三家和流局、本場・供託の上家取り、流し満貫時の親聴牌連荘、供託を除く終局判定を有効化。切り上げ満貫、暗槓国士槍槓、特殊形ダブル役満を無効化。責任払いは大三元・大四喜のみ、複合役満を含む和了全体対象 |
| `cj4_rules_mjsoul()` | 4槓目は嶺上牌の打牌後に流局。槓ドラは後めくり。三家和は3人ロン、責任払いは和了全体対象 |

非対応：

- 三人麻雀（別実装想定）
- 人和、途中流局の詳細差分、ローカル役
- 供託・順位点を含む最終精算
- 複合役満で役満ごとに責任者が異なる責任払い

---

## 互換性 / Compatibility

v3 は破壊的変更です。位置情報の収集APIは `locations` を受け取り、配列と件数をまとめた値を返します。`cj4_player_view` もマスク済みの `locations` を保持する形式へ変更しています。旧収集API、`cj4_make_player_state()`、状態変更用アクセサーとのソース互換・バイナリABI互換は保証しません。

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
- player API: `cj4p_*`

例:

cj4_do_discard cj4_can_ron cj4m_step cj4m_collect_actions cj4p_calculate_shanten

---

## ディレクトリ構成 / Project Structure

```
include/cjong4/core/      core public API
include/cjong4/manager/   manager public API
include/cjong4/player/    player-facing public API
src/core/                 core implementation
src/manager/              manager implementation
src/player/               player-facing implementation
tests/test_support.*      shared test support
tests/core/               core tests
tests/manager/            manager tests
doc/                      architecture and ADRs
cmake/                    CMake package configuration
.github/workflows/        continuous integration
```

---

## ステータス / Status

3.3.0 リリース
3.3.0 released

---

## ライセンス / License

MIT License
