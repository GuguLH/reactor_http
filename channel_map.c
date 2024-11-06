#include "channel_map.h"

channel_map_t *channel_map_init(int size)
{
    channel_map_t *map = (channel_map_t *)malloc(sizeof(channel_map_t));
    map->size = size;
    map->list = (channel_t **)malloc(size * sizeof(channel_t));
    return map;
}

void channel_map_clear(channel_map_t *map)
{
    if (map != NULL)
    {
        for (int i = 0; i < map->size; i++)
        {
            if (map->list[i] != NULL)
            {
                free(map->list[i]);
            }
        }
        free(map->list);
        map->list = NULL;
    }
    map->size = 0;
}

bool channel_map_resize(channel_map_t *map, int new_sz)
{
    if (map->size < new_sz)
    {
        int cur_sz = map->size;
        while (cur_sz > new_sz)
        {
            cur_sz *= 2;
        }
        channel_t **tmp = (channel_t **)realloc(map->list, cur_sz * sizeof(channel_t));
        if (tmp == NULL)
        {
            return false;
        }
        map->list = tmp;
        memset(&map->list[map->size], 0, (cur_sz - map->size) * sizeof(channel_t));
        map->size = cur_sz;
    }
    return true;
}