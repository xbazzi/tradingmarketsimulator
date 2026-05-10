#include "mms/market/Order.hh"

Order::Order(OrderType type, OrderId id, Side side, Price price, Quantity qty)
    : _type{type}, _id{id}, _side{side}, _price{price}, _initial_quantity{qty}
{
}
