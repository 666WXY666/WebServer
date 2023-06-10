#include "../headers/heaptimer.h"

/*
 * 构造函数，预申请一些空间
 */
HeapTimer::HeapTimer()
{
    heap_.reserve(64);
}

/*
 * 析构函数，清空空间
 */
HeapTimer::~HeapTimer()
{
    clear();
}

/*
 * 上移调整堆中元素
 */
void HeapTimer::shiftup_(size_t i)
{
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2; // 父节点
    while (j >= 0)
    {
        // 不需要上移了
        if (heap_[j] < heap_[i])
        {
            break;
        }
        // 交换
        swapNode_(i, j);
        // 更新当前节点
        i = j;
        // 更新父节点
        j = (i - 1) / 2;
    }
}

/*
 * 交换堆中两个节点
 */
void HeapTimer::swapNode_(size_t i, size_t j)
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    // 注意：不要忘记修改map映射
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

/*
 * 下移调整堆中元素
 */
bool HeapTimer::shiftdown_(size_t index, size_t n)
{
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1; // 左子节点
    while (j < n)
    {
        // 有右子节点并且右孩子更小
        if (j + 1 < n && heap_[j + 1] < heap_[j])
        {
            j++;
        }
        // j为最小孩子，与当前节点比较，如果当前更小说明不需要再下移了
        if (heap_[i] < heap_[j])
        {
            break;
        }
        // 交换
        swapNode_(i, j);
        // 更新当前节点
        i = j;
        // 更新左子节点
        j = i * 2 + 1;
    }
    // 返回是否下移
    return i > index;
}

/*
 * 将指定文件描述符id添加一个定时器到堆中，超时时间为timeout，回调函数为cb
 */
void HeapTimer::add(int id, int timeout, const TimeoutCallBack &cb)
{
    assert(id >= 0);
    size_t i;
    // 新节点：插入堆尾，上移调整堆
    if (ref_.count(id) == 0)
    {
        // 插入堆尾
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        // 上移调整堆
        shiftup_(i);
    }
    // 已有结点：更新超时时间，调整堆
    else
    {
        i = ref_[id];
        // 更新超时时间
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        // 上移和下移都尝试
        if (!shiftdown_(i, heap_.size()))
        {
            shiftup_(i);
        }
    }
}

/*
 * 删除指定id结点，并触发回调函数（未使用）
 */
void HeapTimer::doWork(int id)
{
    if (heap_.empty() || ref_.count(id) == 0)
    {
        return;
    }
    // 获取TimerNode
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    // 执行回调函数
    node.cb();
    // 删除TimerNode
    del_(i);
}

/*
 * 删除堆中的指定index节点
 */
void HeapTimer::del_(size_t index)
{
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    // 将要删除的结点换到队尾，堆大小-1，然后调整堆
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n)
    {
        // 交换
        swapNode_(i, n);
        // 如果不需要下移，那么就上移
        if (!shiftdown_(i, n))
        {
            shiftup_(i);
        }
    }
    // 队尾元素删除
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

/*
 * 调整文件描述符id关联的定时器的过期时间为：当前时间+timeout，并下移调整堆
 */
void HeapTimer::adjust(int id, int timeout)
{
    assert(!heap_.empty() && ref_.count(id) > 0);
    // 调整指定id的结点
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);
    // 时间增大，下移调整堆
    shiftdown_(ref_[id], heap_.size());
}

/*
 * 删除堆中所有的超时节点，并触发它们的回调函数
 */
void HeapTimer::tick()
{
    if (heap_.empty())
    {
        return;
    }
    // while遍历
    while (!heap_.empty())
    {
        TimerNode node = heap_.front();
        // 当前时间<节点超时时间，小根堆，后面的节点超时时间更大，都没有超时，直接break
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
        {
            break;
        }
        // 当前时间>=节点超时时间，超时，触发回调函数
        node.cb();
        // 移除超时节点
        pop();
    }
}

/*
 * 删除堆中的第一个节点
 */
void HeapTimer::pop()
{
    assert(!heap_.empty());
    del_(0);
}

/*
 * 清空堆中的元素
 */
void HeapTimer::clear()
{
    ref_.clear();
    heap_.clear();
}

/*
 * 删除所有超时节点，返回最近一个超时节点的超时时间
 */
int HeapTimer::getNextTick()
{
    // 删除堆中所有的超时节点，并触发它们的回调函数
    tick();
    // 初始值为-1，代表epoll_wait阻塞
    size_t res = -1;
    // if只取堆顶
    if (!heap_.empty())
    {
        // 计算小根堆堆顶的超时时长，也就是最小超时时长，用于epoll_wait的超时等待
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if (res < 0)
        {
            res = 0;
        }
    }
    return res;
}
