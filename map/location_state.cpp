#include "location_state.hpp"
#include "drawer_yg.hpp"

#include "../platform/location.hpp"
#include "../platform/platform.hpp"

#include "../indexer/mercator.hpp"

namespace location
{

  State::State() : m_flags(ENone)
  {
  }

  void State::UpdateGps(m2::RectD const & rect)
  {
    m_flags |= EGps;

    m_positionMercator = rect.Center();

    //m_errorRadiusMercator = sqrt(my::sq(rect.SizeX()) + my::sq(rect.SizeY())) / 2;
    m_errorRadiusMercator = rect.SizeX() / 2.0;
  }

  void State::UpdateCompass(CompassInfo const & info)
  {
    m_flags |= ECompass;

    m_headingRad = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading)
        / 180 * math::pi
        - math::pi / 2;  // 0 angle is for North ("up"), but in our coordinates it's to the right.
    // Avoid situations when offset between magnetic north and true north is too small
    static double const MIN_SECTOR_DEG = 10.;
    m_headingHalfSectorRad = (info.m_accuracy < MIN_SECTOR_DEG ? MIN_SECTOR_DEG : info.m_accuracy)
        / 180 * math::pi;
  }

  void State::DrawMyPosition(DrawerYG & drawer, ScreenBase const & screen)
  {
    double pxErrorRadius;
    m2::PointD pxPosition;
    m2::PointD pxShift(screen.PixelRect().minX(), screen.PixelRect().minY());

    if ((m_flags & State::EGps) || (m_flags & State::ECompass))
    {
      pxPosition = screen.GtoP(Position());
      pxErrorRadius = pxPosition.Length(screen.GtoP(Position() + m2::PointD(m_errorRadiusMercator, 0.0)));

      pxPosition -= pxShift;

      if (m_flags & State::EGps)
      {
        // my position symbol
        drawer.drawSymbol(pxPosition, "current-position", yg::EPosCenter, yg::maxDepth);
        // my position circle
        drawer.screen()->fillSector(pxPosition, 0, math::pi * 2, pxErrorRadius,
                                      yg::Color(0, 0, 255, 32),
                                      yg::maxDepth - 3);
        // display compass only if position is available

        double orientationRadius = max(pxErrorRadius, 30 * GetPlatform().VisualScale());

        if (m_flags & State::ECompass)
        {
          drawer.screen()->drawSector(pxPosition,
                m_headingRad - m_headingHalfSectorRad,
                m_headingRad + m_headingHalfSectorRad,
                orientationRadius,
                yg::Color(255, 255, 255, 192),
                yg::maxDepth);
          drawer.screen()->fillSector(pxPosition,
                m_headingRad - m_headingHalfSectorRad,
                m_headingRad + m_headingHalfSectorRad,
                orientationRadius,
                yg::Color(255, 255, 255, 96),
                yg::maxDepth - 1);
        }
      }
    }
  }
}
