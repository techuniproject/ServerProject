#include "pch.h"
#include "Item.h"

atomic<uint64> Item::s_itemidGenerator = 1;

Protocol::ITEM_TYPE Item::GetRandomItemType()
{
  
   static thread_local std::mt19937 rng{ std::random_device{}() };
   std::uniform_int_distribution<int> dist(0, (int)Protocol::ITEM_TYPE::ITEM_TYPE_MAX_CNT - 1);
   return (Protocol::ITEM_TYPE)dist(rng);
  
}
