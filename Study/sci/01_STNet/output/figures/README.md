# 最终结果图（仅 PNG）

50 轮简化实验已完成，以下为正式结果，均为 600 dpi PNG：

1. [空间表达对照图](final_epoch_050/fig1_spatial_FASN.png)：完整病理图像、肿瘤标记、真实 FASN、预测 FASN，展示全部 3 张切片。
2. [测试表现与校准图](final_epoch_050/fig2_heldout_performance.png)：250 个基因的相关性分布、模型与均值基线的配对 RMSE，以及 FASN 散点。
3. [训练曲线图](final_epoch_050/fig3_learning_curves.png)：50 轮训练/测试 MSE 及相关性轨迹。

按要求，已删除预览图、PDF、SVG 和绘图检查中间文件。训练曲线等绘图源数据保留在 `../simplified/figure_source/`，最终预测和指标在 `../simplified/`。方法与重画命令见 [可视化说明](../../06_结果可视化.md)。

结果为单患者简化实验，不是完整 23 位患者交叉验证。
