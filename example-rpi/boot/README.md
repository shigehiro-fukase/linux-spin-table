# ARM Spin Table のための起動設定

## 概要

Linux 起動用コマンドラインで `maxcpus=3` と指定しているのに、
起動時に Linux カーネルが CPU3 の spin-table を解除してしまう現象があります。

CPU3 は動作モード EL1 に遷移します。

Linux は、その後 CPU[0,1,2] で動作し、CPU3 は sleep 状態にするようですが、
自作のプログラムを CPU3 で動作させたい場合には、この挙動は不都合です。

この現象は Linux のバージョンによるかもしれません。<br>
本文章は Raspberry Pi3B, Raspberry Pi OS 6.1.21-v8+ で確認したものです。

## 原因

Linux カーネルソースの spin-table の処理は
device-tree の `cpus` に記述された `cpu@数字` について、
起動 CPU の `0` 以外の全てを spin-table の release 処理するようです。
このとき、`maxcpus=<数字>` の設定は参照されません。

- `smp_spin_table.c`
  - `maxcpus=3` の場合、`smp_spin_table_cpu_prepare(unsigned int cpu)` 関数が 引数 `cpu``1`,`2`,`3` の3回呼ばれる

## 対策

Linux カーネルのソースコードを修正するとか、
起動用の device-tree ファイルを修正するとか、
方法はいくつか考えられますが、
ここでは、できるだけ元のファイルを変更しない方法をとってみました。


### device-tree overlay ファイルの作成

Raspberry Pi3B 起動用の device-tree ファイルをベースに、overlay ファイルを作成します。

- Raspberry Pi3B 起動用の device-tree ファイル: `/boot/bcm2710-rpi-3-b.dtb`

#### DTB を DTS に変換します

```
# dtc -I dtb -O dts -o /boot/bcm2710-rpi-3-b.dtb bcm2710-rpi-3-b.dts
```

#### overlay 用 DTSO の作成

作成された `bcm2710-rpi-3-b.dts` の中の `cpus` の `cpu@3` のあたりの記述をもとに
以下のような内容のファイルを作成します。

`rpi3b-cpu3.dtso`
```
/dts-v1/;
/plugin/;
/{
    fragment@0 {
        target-path="/";
        __overlay__ {
	cpus {
		#address-cells = <0x01>;
		#size-cells = <0x00>;
		enable-method = "brcm,bcm2836-smp";
		phandle = <0x8d>;

		cpu@3 {
			device_type = "cpu";
			compatible = "arm,cortex-a53";
			reg = <0x03>;
			enable-method = "spin-table";
/*
			cpu-release-addr = <0x00 0xf0>;     // ←元の記述 0xf0 を 0xd8 (cpu@0 と同じ設定) にする
*/
			cpu-release-addr = <0x00 0xd8>;
			d-cache-size = <0x8000>;
			d-cache-line-size = <0x40>;
			d-cache-sets = <0x80>;
			i-cache-size = <0x8000>;
			i-cache-line-size = <0x40>;
			i-cache-sets = <0x100>;
			next-level-cache = <0x21>;
			phandle = <0x25>;
		};
	};
        };
    };
};
```

#### DTSO を DTBO に変換します

```
# dtc -I dts -O dto -o rpi3b-cpu3.dtbo rpi3b-cpu3.dtso
```

#### DTBO を配置します

`/boot/overlays/` の中にコピーします。

```
# cp rpi3b-cpu3.dtbo /boot/overlays/`
```

#### 起動用設定の書き換え

`/boot/config.txt` ファイル内に overlay device-tree ファイルの読み込み設定を加筆します。

`/boot/config.txt`
```
[pi3]
dtoverlay=rpi3b-cpu3
```

#### 起動

この設定で起動すると、`cpu@3` の `cpu-release-addr` が `cpu@0` と同じになっていて、
CPU0 は既に起動しているために、SMP CPU起動処理がスキップされる、という効果になります。

dirty hacks とか bad know-how に類すると思います。

#### 注意

Linux 起動後に設定を見てみると `0xd8` になっていて
overlay ファイルに記述した内容が、そのまま参照されています。

```
$ xxd /proc/device-tree/cpus/cpu@3/cpu-release-addr
00000000: 0000 0000 0000 00d8                      ........
```

`cpu@3` を（本当に）起動したい場合に書き換えるべきアドレスは `0xd8` ではなく `0xf0` （元のDTSの記述）です。


