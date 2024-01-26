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
  - device-tree の `cpus` に `cpu@0`, `cpu@1`, `cpu@2`, `cpu@3`の記述がある場合、
    `smp_spin_table_cpu_prepare(unsigned int cpu)` 関数が 引数 `cpu` で `1`,`2`,`3` の3回呼ばれる。
    コマンドラインで `maxcpus=3` の指定があっても 引数 `cpu=3` の呼び出しは行われる。

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



-----

## その他トピック：spin-table のアドレスにアクセスする方法

Linux から spin-table の `release-address` の値を書き換えるには工夫が要ります。

- mmap: 起動用 device-tree `/boot/bcm2710-rpi-3-b.dtb` の書き換えが必要です
- generic uio: 起動用コマンドラインの加筆が必要ですが、DTB は overlay で済みます


## Generic UIO を使う場合

UIO を使うには

- `cmdline.txt` に uio 用の引数を追加する
  - `uio_pdrv_genirq.of_id="generic-uio"`
- DTB を修正して uio 用のノードを作る
  - Linux起動後に overlay で DTBO をロードする方法でもいい

の手順が必要

Linux の `bootargs` が無いと、class 登録はされるが `/dev/uio*` が作られない。


#### `cmdline.txt`例
```
console=serial0,115200 console=tty1 root=PARTUUID=969e7a9d-02 rootfstype=ext4 fsck.repair=yes rootwait quiet splash plymouth.ignore-serial-consoles maxcpus=1 mem=512M nokaslr cpuidle.off=1 rodata=off uio_pdrv_genirq.of_id="generic-uio"
```

#### `dtso` 例
```
/dts-v1/;
/plugin/;
/{
    fragment@0 {
        target-path="/";
        __overlay__ {
            #address-cells = <0x01>;
            #size-cells = <0x01>;

            uio@0 {
                compatible = "generic-uio";
                reg = <0xd8 0x8>;
            };
            uio@1 {
                compatible = "generic-uio";
                reg = <0xe0 0x8>;
            };
            uio@2 {
                compatible = "generic-uio";
                reg = <0xe8 0x8>;
            };
            uio@3 {
                compatible = "generic-uio";
                reg = <0xf0 0x8>;
            };
        };
    };
};
```

#### `dtbo` インストール例
```
dtc -I dts -O dtb -o dts/rpi3b-uio.dtbo dts/rpi3b-uio.dtso
sudo mkdir -p /sys/kernel/config/device-tree/overlays/$$
ls -l /sys/kernel/config/device-tree/overlays/$$
sudo sh -c "cat dts/rpi3b-uio.dtbo > /sys/kernel/config/device-tree/overlays/$$/dtbo"
cat /sys/kernel/config/device-tree/overlays/$$/status
ls /proc/device-tree/
sudo modprobe uio_pdrv_genirq of_id="generic-uio"
ls /dev/uio*
```

### `/dev/mem` で `mmap` したい場合

DTB の先頭に `/memreserve/` の記述があり、`mmap` できなくなっている。

#### `/boot/bcm2710-rpi-3-b.dtb` の DTS（抜粋）
```
dts-v1/;

/memreserve/    0x0000000000000000 0x0000000000001000;
/ {
        compatible = "raspberrypi,3-model-b\0brcm,bcm2837";
        model = "Raspberry Pi 3 Model B";
        #address-cells = <0x01>;
        #size-cells = <0x01>;
        interrupt-parent = <0x01>;
```

`/memreserve/` を除外するとともに、`reserved-memory` を記述すれば `mmap` できる。

#### `/memreserve/` から `0x0...0x100` 範囲を除外（抜粋）
```
/dts-v1/;

/memreserve/ 0x0000000000000100 0x0000000000001000;
/ {
        compatible = "raspberrypi,3-model-b\0brcm,bcm2837";
        model = "Raspberry Pi 3 Model B";
        #address-cells = <0x01>;
        #size-cells = <0x01>;
        interrupt-parent = <0x01>;
```

#### `reserved-memory` に `cpu-spin-table` を加筆（抜粋）
```
        reserved-memory {
                #address-cells = <0x01>;
                #size-cells = <0x01>;
                ranges;
                phandle = <0x38>;

                linux,cma {
                        compatible = "shared-dma-pool";
                        size = <0x4000000>;
                        reusable;
                        linux,cma-default;
                        phandle = <0x39>;
                };

                cpu-spin-table {
                        device-type = "memory";
                        no-map;
                        reg = <0x0 0x100>;
                };
        };
```


