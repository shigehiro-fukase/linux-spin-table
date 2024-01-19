# ARM Spin Table

## 概要

ARMv8 multiprocessor system において、2個目以降の CPU (secondary processors) を起こす方法には `"spin-table"` と `"psci"` の二種類があります。

ここでは

- `"spin-table"` 方式について扱います
  - 有名な例として Raspberry Pi3 と Pi4 が挙げられます
  - `"psci"` とは全く別です。もし、そちらが知りたければ "Power State Coordination Interface", "SMC", "ATF" などのキーワードで調べると良いでしょう
- ARM64 について扱います
  - 32bit モードについては考慮していません
- ターゲットボードで起動中の Linux から停止中の CPU を起こすことを扱います
  - Linux が全ての CPU を SMP で使用している場合は扱えません
    - Linux カーネルコマンドラインで `maxcpus=数字` の数を小さく指定すると良いでしょう
      - これだけでは不十分な場合があります（[RPI3の場合](./example-rpi/boot/README.md)参照）

## "spin-table" 方式について

`"spin-table"` 方式の場合、起動していない CPU は電源が切れているのではなく、以下のような挙動のごく小さなプログラムが動作しているようです。

- `WFI` 命令で休眠する
- 起床したら `release-address` の値を読む
  - 値が `0` なら `WFI` 命令で再び休眠する
  - 値が `0` 以外なら ジャンプする

動作中の CPU から、起動していない CPU を起こすには、次のようにします。

- RAM に実行したいプログラムを転送する
- `release-address` にプログラムの開始アドレスを書き込む
- `SEV` 命令で `WFI` を解除する
  - 注）CPU を指定しませんので `WFI` している全ての CPU が起床します

## フォルダ内の説明

Linux の場合、`release-address` は device-tree に記述されています。

- ARM64 Linux では procfs から device-tree の値を確認できます
  - 2ndary CPU 起動方法が `"spin-table"` 方式かどうか確認する
    - `cat /proc/device-tree/cpus/cpu@#/enable-method`
  - `release-address` は `od -x` や `xxd` コマンドで確認できます
    - `xxd "/proc/device-tree/cpus/cpu@#/cpu-release-addr"`
    - フォルダ内の `./linux-get-release-addr/` プログラムでも確認できます
  - （上述の `#` 部分はCPU番号の数字）

- RAM に実行したいプログラムを転送するには
  - Linux からファイルを RAM に転送するプログラムを ここ（[`linux-load-file`](https://github.com/shigehiro-fukase/linux-load-file)）に作成してあります
  - u-boot や JTAG デバッガ が利用できるなら場合もあります

- `release-address` に値を書き込むために、以下のプログラムを作成してあります
  - Linux 用のモジュール : カーネル空間で mmap して RAM を I/O する 
    - フォルダ内の `./linux-mem-paio/`
  - Linux 用のプログラム : `/dev/mem` や `/dev/uio*` を mmap して RAM を I/O する
    - フォルダ内の `./linux-mod-paio/`
  - 注）いずれの場合もシステム設定や device-tree の記述の変更をしないとうまくいかないことがあります

- `SEV` 命令を発行する
  - Linux 用のプログラムを作成してあります
    - フォルダ内の `./linux-sev/`

- Generic UIO を使用する例
  - Raspberry Pi3/Pi4 で Raspberry Pi OS が CPU[0,1,2] で動作した状態で、CPU3 を起動する実験をしたときの device-tree 設定を置いてあります
    - フォルダ内の `./example-rpi/`


## References

### ARM Support forums の説明
- Setting PC for and waking up secondary cores from the primary core
  - https://community.arm.com/support-forums/f/architectures-and-processors-forum/44183/setting-pc-for-and-waking-up-secondary-cores-from-the-primary-core

### Linux カーネルの devicetree のドキュメント

- https://www.kernel.org/doc/Documentation/devicetree/bindings/arm/cpus.txt
- https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/arm/cpus.yaml

