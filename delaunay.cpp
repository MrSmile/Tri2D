// delaunay.cpp : Delaunay triangulation
//

#include "delaunay.h"
#include "ntl/tree.h"

using namespace NTL;



struct BeachPoint;
struct QueueEvent;

typedef DefaultAllocator<BeachPoint> BeachAlloc;
typedef DefaultAllocator<QueueEvent> EventAlloc;

struct BeachPoint : public TreeNode<BeachPoint>
{
    const Point *pt[2];
    Triangle *tr;
    QueueEvent *ev;

    BeachPoint(const Point *pt1, const Point *pt2, Triangle *t) : tr(t), ev(nullptr)
    {
        pt[0] = pt1;  pt[1] = pt2;
    }

    int cmp(const Vec2D &v) const
    {
        Vec2D d = *pt[1] - *pt[0];
        if(d.y == 0)
        {
            if(d.x <= 0)return 1;
            if(v.y <= pt[0]->y)return v.x > pt[1]->x;
        }
        double y1 = v.y - pt[0]->y, y2 = v.y - pt[1]->y, w = 4 * y1 * y2;
        if(d.x > 0)
        {
            double t = (d.x * d.x - w) / (d.x * (y1 + y2) + sqrt(w * d.sqr()));
            return 2 * v.x > pt[1]->x + pt[0]->x + t * d.y ? -1 : 1;
        }
        else
        {
            double t = d.x * (y1 + y2) - sqrt(w * d.sqr());
            return 2 * v.x > pt[1]->x + pt[0]->x + t / d.y ? -1 : 1;
        }
    }
};

struct QueueEvent : public Vec2D, public TreeNode<QueueEvent>
{
    double yy;
    BeachPoint *seg;
    const Point *pt;

    QueueEvent(const Point *p) : Vec2D(*p), yy(p->y), seg(nullptr), pt(p)
    {
    }

    QueueEvent(BeachPoint *s, const Vec2D &v, double delta) : Vec2D(v), yy(v.y + delta), seg(s), pt(nullptr)
    {
    }

    static QueueEvent *triangle(EventAlloc &alloc, BeachPoint *seg, const Point *pt2)
    {
        if(seg->pt[0] == pt2)return nullptr;
        Vec2D r0 = (*seg->pt[0] + *pt2) / 2, r = *seg->pt[1] - r0;
        Vec2D d = *pt2 - *seg->pt[0];  double s = r % d;  if(s <= 0)return nullptr;
        double dd4 = d.sqr() / 4, h = (r.sqr() - dd4) / s;
        return alloc.create(seg, r0 + (~d) * (h / 2), sqrt(dd4 * (1 + h * h)));
    }

    int cmp(const QueueEvent &qe) const
    {
        if(yy < qe.yy)return -1;  if(yy > qe.yy)return 1;  return x < qe.x ? -1 : 1;
    }
};


inline void connect(Triangle *tr1, int ed1, Triangle *tr2, int ed2)
{
    tr1->next[ed1] = TriRef(tr2, ed2);  tr2->next[ed2] = TriRef(tr1, ed1);
}

inline void connect(Triangle *tr1, int ed1, const TriRef &tr2)
{
    tr1->next[ed1] = tr2;  tr2->next[tr2.index] = TriRef(tr1, ed1);
}


size_t triangulate(Triangle *tr, const Point *point, size_t point_count)
{
    if(point_count < 2)return 0;
    EventAlloc queue;  Tree<QueueEvent, EventAlloc> event(queue);
    for(size_t i = 0; i < point_count; i++)event.insert(queue.create(&point[i]));

    BeachAlloc beach;  Tree<BeachPoint, BeachAlloc> line(beach);
    QueueEvent *pt1 = event.first();  pt1->remove();
    QueueEvent *pt2 = event.first();  pt2->remove();

    tr[0].pt[0] = tr[1].pt[0] = nullptr;
    tr[0].pt[1] = tr[1].pt[2] = pt1->pt;
    tr[0].pt[2] = tr[1].pt[1] = pt2->pt;

    connect(tr, 0, tr + 1, 0);  connect(tr, 1, tr + 1, 2);  connect(tr, 2, tr + 1, 1);

    BeachPoint *bp1 = beach.create(pt1->pt, pt2->pt, tr++);
    //line.insert(bp1, Tree<BeachPoint, BeachAlloc>::Place(nullptr, false));
    line.insert(bp1, line.find_place(Point()));  // TODO
    BeachPoint *bp2 = beach.create(pt2->pt, pt1->pt, tr++);
    line.insert(bp2, Tree<BeachPoint, BeachAlloc>::Place(bp1, true));
    queue.remove(pt1);  queue.remove(pt2);

    while(event)
    {
        QueueEvent *ev = event.first();  ev->remove();
        if(ev->pt)
        {
            OwningTree<BeachPoint>::Place pos = line.find_place(*ev->pt);
            BeachPoint *prev = pos.before(), *next = pos.after();  assert(!pos.node());
            if(!prev)prev = line.last();  if(!next)next = line.first();

            tr[0].pt[0] = tr[1].pt[0] = nullptr;
            tr[0].pt[1] = tr[1].pt[2] = prev->pt[1];
            tr[0].pt[2] = tr[1].pt[1] = ev->pt;

            connect(tr, 0, tr + 1, 0);  connect(tr, 1, tr + 1, 2);
            connect(tr, 2, prev->tr, 1);  connect(next->tr, 2, tr + 1, 1);

            BeachPoint *bp1 = beach.create(prev->pt[1], ev->pt, tr++);  line.insert(bp1, pos);
            BeachPoint *bp2 = beach.create(ev->pt, prev->pt[1], tr++);
            line.insert(bp2, Tree<BeachPoint, BeachAlloc>::Place(bp1, true));

            queue.remove(prev->ev);
            if((prev->ev = QueueEvent::triangle(queue, prev, ev->pt)))event.insert(prev->ev);
            if((bp2->ev = QueueEvent::triangle(queue, bp2, next->pt[1])))event.insert(bp2->ev);
        }
        else
        {
            BeachPoint *prev = ev->seg, *next = ev->seg->next();

            prev->tr->pt[0] = next->pt[1];  next->tr->pt[1] = prev->pt[0];
            connect(prev->tr, 1, next->tr->next[0]);  connect(next->tr, 2, prev->tr->next[2]);
            connect(prev->tr, 2, next->tr, 0);

            next->pt[0] = prev->pt[0];  beach.remove(prev);
            if(BeachPoint *ptr = next->prev())
            {
                queue.remove(ptr->ev);
                if((ptr->ev = QueueEvent::triangle(queue, ptr, next->pt[1])))event.insert(ptr->ev);
            }
            if(BeachPoint *ptr = next->next())
            {
                queue.remove(next->ev);
                if((next->ev = QueueEvent::triangle(queue, next, ptr->pt[1])))event.insert(next->ev);
            }
        }
        queue.remove(ev);
    }

    return 2 * point_count - 2;
}
