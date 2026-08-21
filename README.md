# SlopOS-Deepseek

由 AI 从零独立编写的 x86-64 操作系统,完整项目代码分别存放在 `v1` 和 `v2`
分支。本分支 (`main`) 仅存放版本说明与原始提示词。

## 版本说明 (Versions)

- **v1** — 由 **Deepseek-V4-Flash** 写出。由于没有多模态能力,模型看不到屏幕,
  无法根据画面调试,导致有很多 Bug。
- **v2** — 由 **Deepseek-V4-Flash-Visual-Exp** 编写。有了多模态能力,模型可以
  直接观察运行画面并据此调试,问题少了一点。

| 分支 | 编写模型 | 多模态 | 状况 |
|------|----------|--------|------|
| `v1` | Deepseek-V4-Flash | 无 | 有很多 Bug |
| `v2` | Deepseek-V4-Flash-Visual-Exp | 有 | 问题少了一点 |

## 提示词 (Prompt)

以下提示词是生成 SlopOS 时使用的原始提示词:

```
Create an operating system named SlopOS from an empty repository.

SlopOS must be a real, independently implemented operating system that boots in QEMU. Its primary user interface must be graphical: normal boot should enter an interactive graphical desktop with working keyboard and mouse input, windows, and at least a terminal or comparable way to interact with the system. A boot logo, static framebuffer image, text console, or graphical program secretly running on another operating system does not count.

SlopOS should implement as much useful operating-system functionality as you can, including memory management, interrupts, processes or tasks, files, executable loading, input, graphics, and user programs. It should also attempt compatibility with the Linux x86-64 userspace ABI, with the goal of running selected unmodified Linux executables. Clearly distinguish partial compatibility from general compatibility, and verify compatibility by actually running unchanged binaries.

All original SlopOS code must be released under the 0BSD license. The distributed system must remain compatible with public-domain-style reuse. Do not copy, translate, or incorporate GPL, LGPL, AGPL, or other copyleft implementation code. Permissively licensed dependencies may be used when necessary, but document their origin and licenses. Do not disguise an existing kernel, Linux distribution, emulator-hosted application, or ported operating system as SlopOS.

You have no human collaborator. After this prompt, you will receive no answers, confirmations, design decisions, debugging advice, or further instructions. Do not ask questions or wait for approval. Explore the environment, make all technical decisions yourself, and revise those decisions when evidence shows they are poor.

The available machine is macOS on arm64 with QEMU, LLVM, GCC, and ordinary development tools. OrbStack provides Debian 13 virtual machines for both arm64 and x86-64. You may use any of these environments and install or build additional tools as needed. Choose the target architecture and development workflow that best serve the stated goals, though Linux x86-64 ABI compatibility must remain an explicit objective.

Work autonomously and iteratively. Repeatedly design, implement, build, boot, test, inspect the result, debug failures, and improve it. Do not stop merely because the project compiles, boots, displays graphics, or passes an early demonstration. Continue until the execution environment or available budget stops you, or until further work is no longer technically meaningful.

Keep the repository reproducible and auditable. Maintain build and run instructions, license and dependency records, an honest account of implemented and missing functionality, automated tests where practical, and evidence of actual QEMU execution. Use version control and preserve meaningful development history.

Prefer working, observable behavior over large quantities of generated code or untested subsystem skeletons. Do not claim that a feature works unless you have executed and verified it.

Begin now.
```

## 许可

全部代码以 0BSD 许可发布,详见各分支中的 LICENSE 文件。
