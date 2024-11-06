#ifndef __CHANNEL_MAP_H__
#define __CHANNEL_MAP_H__

#include "channel.h"

typedef struct channel_map
{
    int size;
    channel_t **list;
} channel_map_t;

// 初始化
channel_map_t *channel_map_init(int size);
// 清空
void channel_map_clear(channel_map_t *map);
// 扩容
bool channel_map_resize(channel_map_t *map, int new_sz);

#endif