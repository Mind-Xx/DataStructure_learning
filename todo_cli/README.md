# Todo CLI（C++ 待办清单）

一个练习数据结构的小项目：用 `vector` 保存任务，用另一个 `vector` 当作栈实现撤销。

## 编译与运行

需要支持 C++17 的编译器。在本文件夹打开终端，运行：

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp -o todo_cli.exe
.\todo_cli.exe
```

## 命令

```text
add 学习链表
add 写一道栈的练习题
list
done 1
delete 2
undo
quit
```

每个任务都有固定 ID。删除任务后，其他任务的 ID 不会改变。`undo` 可以连续使用，但只撤销**本次运行期间**的修改；重新启动后撤销记录会清空。

任务会在每次修改后保存到程序旁边的 `tasks.txt`。重新启动程序时会自动读取。这个文件由程序管理，不建议手动修改。

## 可以自己继续练习

1. 添加 `edit ID 新内容`，并让它支持撤销。
2. 添加 `pending` 命令，只显示未完成的任务。
3. 让 `undo` 历史也能在程序重启后保留。
