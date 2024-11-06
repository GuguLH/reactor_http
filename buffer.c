#include "buffer.h"

buffer_t *buffer_init(int size)
{
    buffer_t *buffer = (buffer_t *)malloc(sizeof(buffer_t));
    if (buffer != NULL)
    {
        buffer->data = (char *)malloc(size);
        buffer->capacity = size;
        buffer->read_pos = buffer->write_pos = 0;
        memset(buffer->data, 0, size);
    }
    return buffer;
}

void buffer_destory(buffer_t *buf)
{
    if (buf != NULL)
    {
        if (buf->data != NULL)
        {
            free(buf->data);
        }
        free(buf);
    }
}

void buffer_resize(buffer_t *buf, int size)
{
    // 1 内存够用-不需要扩容
    if (buffer_writeable_size(buf) >= size)
    {
        return;
    }
    // 2 内存需要合并才够用-不需要扩容
    // 剩余可写的内存 + 已读的内存
    else if (buf->read_pos + buffer_writeable_size(buf) >= size)
    {
        int readable = buffer_readable_size(buf);
        memcpy(buf->data, buf->data + buf->read_pos, readable);
        buf->read_pos = 0;
        buf->write_pos = readable;
    }
    // 3 内存不够用-扩容
    else
    {
        void *tmp = realloc(buf->data, buf->capacity + size);
        if (tmp == NULL)
        {
            return;
        }
        memset(tmp + buf->capacity, 0, size);
        // 更新数据
        buf->data = tmp;
        buf->capacity += size;
    }
}

int buffer_writeable_size(buffer_t *buf)
{
    return buf->capacity - buf->write_pos;
}

int buffer_readable_size(buffer_t *buf)
{
    return buf->write_pos - buf->read_pos;
}

int buffer_append_data(buffer_t *buf, const char *data, int size)
{
    if (buf == NULL || data == NULL || size <= 0)
    {
        return -1;
    }
    // 扩容
    buffer_resize(buf, size);
    // 数据拷贝
    memcpy(buf->data + buf->write_pos, data, size);
    buf->write_pos += size;
    return 0;
}

int buffer_append_string(buffer_t *buf, const char *data)
{
    int size = strlen(data);
    int ret = buffer_append_data(buf, data, size);
    return ret;
}

int buffer_socket_read(buffer_t *buf, int fd)
{
    struct iovec vec[2];
    int writeable = buffer_writeable_size(buf);
    vec[0].iov_base = buf->data + buf->write_pos;
    vec[0].iov_len = writeable;

    char *tmp_buf = (char *)malloc(40960);
    vec[1].iov_base = tmp_buf;
    vec[1].iov_len = 40960;
    int ret = readv(fd, vec, 2);
    if (ret == -1)
    {
        return -1;
    }
    else if (ret <= writeable)
    {
        buf->write_pos += ret;
    }
    else
    {
        buf->write_pos = buf->capacity;
        buffer_append_data(buf, tmp_buf, ret - writeable);
    }
    free(tmp_buf);
    return ret;
}

char *buffer_find_crlf(buffer_t *buf)
{
    char *ptr = memmem(buf->data + buf->read_pos, buffer_readable_size(buf), "\r\n", 2);
    return ptr;
}

int buffer_send_data(buffer_t *buf, int socket)
{
    // 判断有无数据
    int readable = buffer_readable_size(buf);
    if (readable > 0)
    {
        Debug("发送数据: %s", buf->data + buf->read_pos);
        int count = send(socket, buf->data + buf->read_pos, readable, MSG_NOSIGNAL);
        if (count)
        {
            buf->read_pos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
