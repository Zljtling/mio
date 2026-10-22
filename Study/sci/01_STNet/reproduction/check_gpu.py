import json
from pathlib import Path
import torch
import torchvision

assert torch.cuda.is_available(), 'CUDA unavailable'
torch.set_num_threads(4)
torch.manual_seed(0)
model = torchvision.models.densenet121(weights=None)
model.classifier = torch.nn.Linear(model.classifier.in_features, 250)
model = model.cuda().train()
x = torch.randn(2, 3, 224, 224, device='cuda')
y = torch.randn(2, 250, device='cuda')
optimizer = torch.optim.SGD(model.parameters(), lr=1e-6, momentum=0.9)
loss = (model(x) - y).square().mean()
loss.backward()
optimizer.step()
torch.cuda.synchronize()
report = dict(torch=torch.__version__, torchvision=torchvision.__version__,
              cuda=torch.version.cuda, gpu=torch.cuda.get_device_name(0),
              capability=torch.cuda.get_device_capability(0),
              loss=float(loss.detach()), dense121_backward='passed')
print(json.dumps(report, indent=2))
reports = Path(__file__).resolve().parent / 'reports'
reports.mkdir(parents=True, exist_ok=True)
(reports / 'gpu-check.json').write_text(json.dumps(report, indent=2))
