# ST-Net 项目文件导览

本项目复现 ST-Net（2020）。**50 轮简化实验、评估和结果绘图已完成**：22 位患者训练，留出 `BC23209` 测试，不进行内部四折验证。正式结果图仅保留 PNG。

## 先看哪些文件

| 文档 | 用途 |
| --- | --- |
| [01_STNet.pdf](01_STNet.pdf) | 原始论文 |
| [02_STNet.md](02_STNet.md) | 论文阅读笔记，保留原文 |
| [05_复现记录.md](05_复现记录.md) | 环境、实验设置、结果和操作说明 |
| [06_结果可视化.md](06_结果可视化.md) | 参考论文的绘图设计、最终 PNG 与重画说明 |
| 本文档 | 目录结构、文件用途及保留原因 |

## 当前目录

```text
01_STNet/
├── README.md                         项目导览
├── 01_STNet.pdf                       原始论文
├── 02_STNet.md                        论文阅读笔记
├── 05_复现记录.md                     复现记录
├── ST-Net/                           作者代码及本地兼容性修复
├── data/                             原始数据、训练数据、预训练权重
├── output/
│   ├── simplified/                   完成的 50 轮实验、最终模型与绘图源数据
│   ├── figures/                      仅保留第 50 轮正式 PNG 图
└── reproduction/
    ├── first_experiment.py            后台管理、结束状态和自动评估
    ├── run_simplified.sh              当前实验训练入口
    ├── evaluate.py                    从预测计算 RMSE、Pearson
    ├── fetch_data.py                  下载、校验并整理数据
    ├── prepare_streaming.py           逐切片预处理及表达统计
    ├── check_gpu.py                   GPU 前向、反向传播验证
    ├── run_pilot.sh                   可选：1 轮试跑脚本（历史结果已清理）
    ├── run_full_cv.sh                 可选：完整交叉验证，当前未运行
    ├── environment/                  环境记录
    ├── manifests/                    数据下载清单与 SHA-256
    ├── mappings/                     基因名称映射的原始归档
    ├── patches/                      作者代码兼容性修改补丁
    ├── reports/                      数据、GPU 与最终绘图验证报告
    └── logs/                         数据准备日志
```

## 完成的实验与结果

训练于 2026-10-22 20:03 完成，耗时约 80 分钟。942 个测试点，250 个基因。测试 RMSE 为 0.80789，均值基线 RMSE 为 0.84260，Pearson 中位数为 0.11999，正相关基因为 186/250。

| 路径 | 用途 |
| --- | --- |
| [结果图导航](output/figures/README.md) | 三张正式 PNG 图 |
| `output/simplified/checkpoints/epoch_50.pt` | 最终模型与优化器状态 |
| `output/simplified/pred_50.npz` | 完整最终预测、真实表达和位置 |
| [pred_50.summary.json](output/simplified/pred_50.summary.json) | 指标汇总 |
| [pred_50.metrics.csv](output/simplified/pred_50.metrics.csv) | 逐基因指标 |
| `output/simplified/figure_source/` | 完整 50 轮历史、绘图源数据和清单 |
| `output/simplified/train.log`、`console.log`、`status.json` | 训练日志与完成状态 |

首次 50 轮简化实验已完成。后续诊断任务已停止并清理，当前没有训练任务。中间第 1–49 轮预测已删除，保留第 50 轮预测和全部历史曲线数据。完整训练设置和复现方法见 [05_复现记录.md](05_复现记录.md)。

## 数据：哪些必须保留

| 路径 | 内容及保留原因 |
| --- | --- |
| `data/downloads/` | 273 个官方原始文件，用于校验和重新整理 |
| `data/metadata.csv` | 患者、分型、切片和文件对应关系 |
| `data/hist2tscript/` | 按分型 / 患者整理的表达、坐标、标记、JPEG 和训练用 TIFF |
| `data/hist2tscript-patch/` | 30,627 个点的 NPZ、基因列表、分型、表达均值与中位数 |
| `data/torch/hub/checkpoints/` | ImageNet 预训练 DenseNet-121 权重 |

主要磁盘占用来自训练所需的分块 TIFF 图像。部分原始 JPEG、计数文件在两个目录中使用硬链接，共享磁盘内容；不要修改任意一侧的文件内容。数据已准备完毕，不需要重复下载或预处理。

单张切片以 `BC23209_C1` 为例：`.tsv.gz` 是基因计数，`.spots.txt` 是网格和像素坐标，`_Coords.tsv` 是肿瘤标记，`.jpg` 是原图，`.tif` 供训练读取图块。处理目录的 `C1_19_5.npz` 保存一个点的表达和位置，不包含整张图像。

当前数据为 23 位患者、68 张切片、30,627 个点、26,933 个基因，与论文统计存在差异，详见复现记录。

## 作者代码：主要入口

| 文件 | 作用 |
| --- | --- |
| `ST-Net/README.md` | 作者原始说明 |
| `ST-Net/stnet.cfg` | 本机数据路径；移动项目时需修改绝对路径 |
| `ST-Net/stnet/parser.py` | 命令行参数 |
| `ST-Net/stnet/datasets/spatial.py` | 点数据、图像块和标签归一化 |
| `ST-Net/stnet/cmd/run_spatial.py` | 训练、测试、预测与 checkpoint 保存 |
| `ST-Net/stnet/utils/nn.py` | 网络输出层等辅助函数 |
| `ST-Net/stnet/utils/ensembl.tsv` | Ensembl ID 与基因符号映射 |
| `ST-Net/bin/create_tifs.sh` | 无损分块 TIFF 转换 |
| `ST-Net/bin/cross_validate.py` | 可选完整交叉验证 |
| `ST-Net/setup.py`、`setup.cfg` | 安装与依赖配置 |
| `ST-Net/.git/` | 原始版本和本地修改追溯 |

作者仓库内的其他分析脚本保留，供后续完整复现使用。Python 字节码和 `ensembl.pkl` 是可再生成的缓存；运行时出现这些文件属于正常情况。

## 环境与追溯材料

| 目录 | 主要文件 |
| --- | --- |
| `reproduction/environment/` | `environment-export.yml`、`conda-linux-64.lock`、`pip-freeze.txt` |
| `reproduction/manifests/` | `dataset-manifest-v5.json`：下载地址、大小与校验值 |
| `reproduction/mappings/` | HGNC 2020-07-01 基因映射原始归档 |
| `reproduction/patches/` | `upstream-compatibility.patch`：对作者已跟踪代码的修改 |
| `reproduction/reports/` | 数据统计、GPU 验证、数据完整性抽检、本次清理报告 |
| `reproduction/logs/` | 下载、预处理、图像转换的日志 |

Miniconda 位于 `/home/mio/tools/miniconda3`，环境名 `STNet`，独立于项目目录。

## 可视化结果与最新清理

正式图仅保留 `output/figures/final_epoch_050/` 中的三张 PNG，见 [结果图导航](output/figures/README.md)。已删除第 8 轮预览、PDF/SVG、绘图检查中间文件和第 1–49 轮预测。最终模型、预测、指标和完整 50 轮曲线数据仍保留。原始论文 PDF 不属于结果图，予以保留。

当前仅保留首次完成的 50 轮实验结果及三张最终 PNG。绘图验证记录为 `reproduction/reports/final-figure-validation.json`。

