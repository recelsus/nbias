# nbias

ローカルで動作するCLIベースの暗号化メモツール。
GitHub上の平文のまま置きたくない軽量な個人用テキストを想定しています
認証情報や機密情報のようなケースは想定していません

## Overview

- 暗号化後の拡張子は`.knty`です。上書きではなくファイル名に追記(`recipe.md` → `recipe.md.knty`)。
- 元のファイル名はヘッダーに保存されるため、`.knty`ファイルがリネームされても復号時に元の名前へ復元。
- パスワード保護は既定でOFFです。`-K`を付けずに暗号化すると、`nbias`バイナリに埋め込まれた固定鍵が使われます。

## Build

C++23対応コンパイラ、CMake 3.22以上、libsodium(`libsodium-dev`/`libsodium`等)が必要。

```bash
cmake -S . -B build
cmake --build build
```

`nbias`バイナリは`build/nbias`に生成されます。

### Test

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```
## Usage

```
nbias enc <path...> [--output-dir <dir>] [-K] [--key|-k <value>] [--no-password] [--kdf-profile <fast|balanced|hardened>]
nbias dec <path...> [--output-dir <dir>] [-K] [--key|-k <value>]
nbias edit <path.knty> [--editor <cmd>] [-K] [--key|-k <value>] [--yes]
nbias info <path.knty>
```

### `enc`

```bash
nbias enc unity.md                       # -> unity.md.knty (パスワードなし)
nbias enc unity.md -K --key idolic       # -> unity.md.knty (パスワード保護あり)
nbias enc me.md you.md --output-dir out  # 複数ファイル、出力先はフラットな1ディレクトリ
```

- 対象が既に`.knty`の場合はスキップ(二重暗号化防止)。
- 出力先の`.knty`が既に存在する場合は、上書き前に確認。
- `--kdf-profile`はパスワード保護ありのボルトにのみ影響します。既定値は`fast`。

### `dec`

```bash
nbias dec note.md.knty
nbias dec note.md.knty --key hunter2 --output-dir restored
```

- 復元後のファイル名は、暗号化されているファイル名から`.knty`を外したものではなく、ヘッダーに保存された元のファイル名を使用します
- 復元先に同名ファイルが既に存在する場合: 内容が完全一致すれば無音で上書き(実質no-op)、 異なれば`[y/N]`で確認します、Nでは連番の別名(`unity.01.md`、`unity.02.md`、…)で保存。
- `-K`は このボルトはパスワード保護されているはず という指定です。ヘッダーが実際にはパスワードなしと示している場合は、埋め込み鍵でそのまま復号、パスワード不要だった旨を表示します。
- ヘッダーがパスワード保護ありを示している場合は、後述の順序でパスワードを解決し、誤っていれば対話入力でリトライします。

### `edit`

```bash
nbias edit note.md.knty
nbias edit note.md.knty --key hunter2 --editor "code --wait"
```
- 暗号化ファイルのパスをそのまま受け取ります。`edit`は平文ファイル名から`.knty`パスを推測したり、
- 一時ファイルに復号してエディタを起動、終了後は元の認証方式/KDFプロファイルを維持したまま新しいsalt/nonceを生成して元のボルトに上書き再暗号化します。
- 編集で内容が変わらなかった場合はそのまま。
- エディタの決定順序: `--editor` > `$VISUAL` > `$EDITOR` > `nvim` > `vim` > `vi` > `nano`

### `info`

```bash
nbias info note.md.knty
```
フォーマットバージョン、認証方式(パスワードあり/なし)、KDFプロファイル(パスワード保護時のみ)、元のファイル名を、すべてヘッダーから(パスワード不要で)表示します。

## Password

パスワードが必要な場面(`-K`指定時 or ヘッダーがパスワード保護あり)、以下の順に試行します。

1. コマンドラインの`--key`/`-k`
2. カレントディレクトリの`.env`内の`NBIAS_KEY`
3. 環境変数`NBIAS_KEY`
4. 対話入力(エコー非表示、最大3回リトライ)

## `.env`

`nbias`を実行するディレクトリに`.env`ファイルを作成します。

```bash
# .env
OUTPUT=encrypted
INPUT=.
NBIAS_KEY=love_kenty
```

- `OUTPUT`: `enc`の出力先ベースディレクトリ。入力側のサブディレクトリ構造がそのまま再現(`docs/notes/file.md` → `encrypted/docs/notes/file.md.knty`)。
- `INPUT`: `dec`が復元する先のベースディレクトリ。`OUTPUT`によるマッピングを反転します(`nbias dec encrypted/docs/notes/file.md.knty` → `./docs/notes/file.md`)。
- コマンドラインの`--output-dir`は両方より優先、サブディレクトリ再現はされません。
- `NBIAS_KEY`: 上記のパスワード解決順序を参照。

### `.gitignore`

```gitignore
*.md
!*.md.knty
.env
```

## License

nbias is licensed under the MIT License. See `LICENSE`.

## Third-Party Notices

Third-party dependency and tooling notices are listed in
`THIRD_PARTY_NOTICES.md`.

