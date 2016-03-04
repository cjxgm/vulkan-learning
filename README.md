# vulkan learning results

A code collection as the result of my learning the Vulkan API.

## License

Licensed under The MIT License.

Copyright (C) Giumo Clanjor (哆啦比猫/兰威举), 2016.

## Compilation

Each directory is compiled separately and manually, with [cjxgm/makeless](https://github.com/cjxgm/makeless/blob/master/makeless).

### Install Build System

You will need `Perl5` and standard compilation suite on Linux (like `pacman -Syu base-devel`).

```bash
curl https://github.com/cjxgm/makeless/blob/master/makeless > /tmp/ml
sudo cp /tmp/ml /usr/local/bin
```

### Example: Build 00-info

```bash
cd 00-info
ml main.cc
```

