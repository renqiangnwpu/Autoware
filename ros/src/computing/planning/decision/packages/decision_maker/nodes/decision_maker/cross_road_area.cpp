#include <amathutils.hpp>
#include <cmath>
#include <cross_road_area.hpp>

namespace decision_maker
{
#define TARGET_WAYPOINTS_NUM 15  // need to change rosparam
CrossRoadArea *CrossRoadArea::findClosestCrossRoad(const autoware_msgs::lane &_finalwaypoints,
                                                   std::vector<CrossRoadArea> &intersects)
{
  CrossRoadArea *_area = nullptr;

  amathutils::point _pa;
  amathutils::point _pb;

  double _min_distance = DBL_MAX;

  if (!_finalwaypoints.waypoints.empty())
  {
    _pa.x = _finalwaypoints.waypoints[TARGET_WAYPOINTS_NUM].pose.pose.position.x;
    _pa.y = _finalwaypoints.waypoints[TARGET_WAYPOINTS_NUM].pose.pose.position.y;
    _pa.z = 0.0;
  }

  for (size_t i = 0; i < intersects.size(); i++)
  {
    _pb.x = intersects[i].bbox.pose.position.x;
    _pb.y = intersects[i].bbox.pose.position.y;

    _pb.z = 0.0;

    double __temp_dis = amathutils::find_distance(&_pa, &_pb);

    intersects[i].bbox.label = 0;
    if (_min_distance >= __temp_dis)
    {
      _area = &intersects[i];
      _min_distance = __temp_dis;  //
    }
  }

  if (_area)
  {
    _area->bbox.label = 3;
  }

  return _area;
}

std::vector<geometry_msgs::Point> convhull(const CrossRoadArea *_TargetArea)
{
  std::vector<int> enablePoints;

  // Jarvis's March algorithm
  size_t l = 0;
  for (auto i = begin(_TargetArea->points); i != end(_TargetArea->points); i++)
  {
    if (i->x < _TargetArea->points.at(l).x)
    {
      l = std::distance(begin(_TargetArea->points), i);
    }
  }

  size_t p = l;
  size_t q;

  do
  {
    q = (p + 1) % _TargetArea->points.size();
    for (size_t i = 0; i < _TargetArea->points.size(); i++)
    {
      geometry_msgs::Point pp = _TargetArea->points.at(p);
      geometry_msgs::Point pi = _TargetArea->points.at(i);
      geometry_msgs::Point pq = _TargetArea->points.at(q);
      if ((pi.y - pp.y) * (pq.x - pi.x) - (pi.x - pp.x) * (pq.y - pi.y) < 0)
      {
        q = i;
      }
    }
    enablePoints.push_back(p);
    p = q;
  } while (p != l);

  std::vector<geometry_msgs::Point> point_arrays;
  for (auto p = begin(_TargetArea->points); p != end(_TargetArea->points); p++)
  {
    for (auto &en : enablePoints)
    {
      if (std::distance(begin(_TargetArea->points), p) == en)
      {
        point_arrays.push_back(*p);
      }
    }
  }
  return point_arrays;
}

bool CrossRoadArea::isInsideArea(const CrossRoadArea *_TargetArea, geometry_msgs::Point pt)
{
  std::vector<geometry_msgs::Point> point_arrays = convhull(_TargetArea);

  double rad = 0.0;
  for (auto it = begin(point_arrays); it != end(point_arrays); ++it)
  {
    auto it_n = it + 1;
    if (it == --point_arrays.end())
    {
      it_n = point_arrays.begin();
    }

    double ax = it->x - pt.x;
    double ay = it->y - pt.y;
    double bx = it_n->x - pt.x;
    double by = it_n->y - pt.y;
    double cos_ = (ax * bx + ay * by) / (sqrt(ax * ax + ay * ay) * sqrt(bx * bx + by * by));
    if (cos_ > 1.0)
    {
      cos_ = 1;
    }
    else if (cos_ < -1.0)
    {
      cos_ = -1.0;
    }

    // deg += std::acos(cos_)? std::acos(cos_)/ M_PI * 180.0 : 0.0;
    rad += std::acos(cos_) ? std::acos(cos_) : 0.0;
  }
  if (fabs((2 * M_PI) - rad) <= 0.35 /*about 30 degree*/)
  {
    return true;
  }

  return false;
}
}
