# Architecture Decision Records

このディレクトリには、cjong4の重要なアーキテクチャ上の意思決定を記録します。ADRを採用する判断と運用方法は[ADR 0000](0000-adopt-architecture-decision-records.md)を正とします。

## 一覧

| 番号 | タイトル | ステータス |
| --- | --- | --- |
| [0000](0000-adopt-architecture-decision-records.md) | アーキテクチャ上の意思決定をADRとして記録する | 採用 |
| [0001](0001-use-locations-as-canonical-tile-state.md) | locationsを牌配置の正規状態にする | 採用 |
| [0002](0002-use-value-based-pure-state-transitions.md) | 値ベースの純粋な状態遷移を使用する | 採用 |
| [0003](0003-provide-masked-player-views.md) | プレイヤーごとにマスク済みビューを提供する | 採用 |
| [0004](0004-validate-call-legality-in-core.md) | 鳴き後の打牌可能性をcoreで検証する | 採用 |

## 追加方法

1. 未使用の4桁の連番と、内容を表す英語のkebab-caseをファイル名にします。
2. タイトルと本文は日本語で記述します。
3. Nygardのシンプルな形式に従い、「ステータス」「コンテキスト」「決定」「結果」の4見出しを使用します。
4. 一覧へ追加し、関連するアーキテクチャ文書やADRからリンクします。
5. 以前の判断を置き換える場合は新しいADRを追加し、以前のADRのステータスを「置換済み」にします。
