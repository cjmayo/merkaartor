// Generic Geometry Library
//
// Copyright Barend Gehrels 1995-2009, Geodan Holding B.V. Amsterdam, the Netherlands.
// Copyright Bruno Lalande 2008, 2009
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GGL_ALGORITHMS_SECTIONALIZE_HPP
#define GGL_ALGORITHMS_SECTIONALIZE_HPP

#include <cstddef>
#include <vector>

#include <boost/concept_check.hpp>
#include <boost/concept/requires.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>

#include <ggl/algorithms/assign.hpp>
#include <ggl/algorithms/combine.hpp>

#include <ggl/core/access.hpp>
#include <ggl/core/exterior_ring.hpp>

#include <ggl/iterators/point_const_iterator.hpp>

#include <ggl/util/assign_box_corner.hpp>
#include <ggl/util/math.hpp>
#include <ggl/geometries/segment.hpp>


/*!
\defgroup sectionalize sectionalize: split a geometry (polygon, linestring, etc)
    into monotonic sections

\par Geometries:
- LINESTRING:
- RING:
- POLYGON:
- BOX
*/

namespace ggl
{


/*!
    \brief Structure containing section information
    \details Section information consists of a bounding box, direction
        information (if it is increasing or decreasing, per dimension),
        index information (begin-end, ring, multi) and the number of
        segments in this section

    \tparam Box box-type
    \tparam DimensionCount number of dimensions for this section
    \ingroup sectionalize
 */
template <typename Box, std::size_t DimensionCount>
struct section
{
    typedef Box box_type;

    int directions[DimensionCount];
    int ring_index;
    int multi_index;
    Box bounding_box;

    int begin_index;
    int end_index;
    std::size_t count;
    std::size_t range_count;
    bool duplicate;
    int non_duplicate_index;


    inline section()
        : ring_index(-99)
        , multi_index(-99)
        , begin_index(-1)
        , end_index(-1)
        , count(0)
        , range_count(0)
        , duplicate(false)
        , non_duplicate_index(-1)
    {
        assign_inverse(bounding_box);
        for (register std::size_t i = 0; i < DimensionCount; i++)
        {
            directions[i] = 0;
        }
    }
};


/*!
    \brief Structure containing a collection of sections
    \note Derived from a vector, proves to be faster than of deque
    \note vector might be templated in the future
    \ingroup sectionalize
 */
template <typename Box, std::size_t DimensionCount>
struct sections : std::vector<section<Box, DimensionCount> >
{
    typedef Box box_type;
    static const std::size_t value = DimensionCount;
};


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace sectionalize {

template <typename Segment, std::size_t Dimension, std::size_t DimensionCount>
struct get_direction_loop
{
    typedef typename coordinate_type<Segment>::type coordinate_type;

    static inline void apply(Segment const& seg,
                int directions[DimensionCount])
    {
        coordinate_type const diff =
            ggl::get<1, Dimension>(seg) - ggl::get<0, Dimension>(seg);

        directions[Dimension] = diff > 0 ? 1 : (diff < 0 ? -1 : 0);

        get_direction_loop
            <
                Segment, Dimension + 1, DimensionCount
            >::apply(seg, directions);
    }
};

template <typename Segment, std::size_t DimensionCount>
struct get_direction_loop<Segment, DimensionCount, DimensionCount>
{
    static inline void apply(Segment const& seg,
                int directions[DimensionCount])
    {
        boost::ignore_unused_variable_warning(seg);
        boost::ignore_unused_variable_warning(directions);
    }
};

template <typename T, std::size_t Dimension, std::size_t DimensionCount>
struct copy_loop
{
    static inline void apply(const T source[DimensionCount],
                T target[DimensionCount])
    {
        target[Dimension] = source[Dimension];
        copy_loop<T, Dimension + 1, DimensionCount>::apply(source, target);
    }
};

template <typename T, std::size_t DimensionCount>
struct copy_loop<T, DimensionCount, DimensionCount>
{
    static inline void apply(const T source[DimensionCount],
                T target[DimensionCount])
    {
        boost::ignore_unused_variable_warning(source);
        boost::ignore_unused_variable_warning(target);
    }
};

template <typename T, std::size_t Dimension, std::size_t DimensionCount>
struct compare_loop
{
    static inline bool apply(const T source[DimensionCount],
                const T target[DimensionCount])
    {
        bool const not_equal = target[Dimension] != source[Dimension];

        return not_equal
            ? false
            : compare_loop
                <
                    T, Dimension + 1, DimensionCount
                >::apply(source, target);
    }
};

template <typename T, std::size_t DimensionCount>
struct compare_loop<T, DimensionCount, DimensionCount>
{
    static inline bool apply(const T source[DimensionCount],
                const T target[DimensionCount])
    {
        boost::ignore_unused_variable_warning(source);
        boost::ignore_unused_variable_warning(target);

        return true;
    }
};


template <typename Segment, std::size_t Dimension, std::size_t DimensionCount>
struct check_duplicate_loop
{
    typedef typename coordinate_type<Segment>::type coordinate_type;

    static inline bool apply(Segment const& seg)
    {
        coordinate_type const diff =
            ggl::get<1, Dimension>(seg) - ggl::get<0, Dimension>(seg);

        if (! ggl::math::equals(diff, 0))
        {
            return false;
        }

        return check_duplicate_loop
            <
                Segment, Dimension + 1, DimensionCount
            >::apply(seg);
    }
};

template <typename Segment, std::size_t DimensionCount>
struct check_duplicate_loop<Segment, DimensionCount, DimensionCount>
{
    static inline bool apply(Segment const&)
    {
        return true;
    }
};

template <typename T, std::size_t Dimension, std::size_t DimensionCount>
struct assign_loop
{
    static inline void apply(T dims[DimensionCount], int const value)
    {
        dims[Dimension] = value;
        assign_loop<T, Dimension + 1, DimensionCount>::apply(dims, value);
    }
};

template <typename T, std::size_t DimensionCount>
struct assign_loop<T, DimensionCount, DimensionCount>
{
    static inline void apply(T dims[DimensionCount], int const)
    {
        boost::ignore_unused_variable_warning(dims);
    }
};


template
<
    typename Range,
    typename Point,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize_range
{
    static inline void apply(Range const& range, Sections& sections,
                int ring_index = -1, int multi_index = -1)
    {
        typedef segment<const Point> segment_type;

        std::size_t const n = boost::size(range);
        if (n == 0)
        {
            // Zero points, no section
            return;
        }

        if (n == 1)
        {
            // Line with one point ==> no sections
            return;
        }

        int i = 0;
        int ndi = 0; // non duplicate index

        typedef typename boost::range_value<Sections>::type sections_range_type;
        sections_range_type section;

        typedef typename boost::range_const_iterator<Range>::type iterator_type;
        iterator_type it = boost::begin(range);

        for(iterator_type previous = it++;
            it != boost::end(range);
            previous = it++, i++)
        {
            segment_type s(*previous, *it);

            int direction_classes[DimensionCount] = {0};
            get_direction_loop
                <
                    segment_type, 0, DimensionCount
                >::apply(s, direction_classes);

            // if "dir" == 0 for all point-dimensions, it is duplicate.
            // Those sections might be omitted, if wished, lateron
            bool check_duplicate = true; //?
            bool duplicate = false;

            if (check_duplicate && direction_classes[0] == 0)
            {
                // Recheck because all dimensions should be checked,
                // not only first one,
                // Note that DimensionCount might be < dimension<P>::value
                if (check_duplicate_loop
                    <
                        segment_type, 0, ggl::dimension<Point>::type::value
                    >::apply(s)
                    )
                {
                    duplicate = true;

                    // Change direction-info to force new section
                    // Note that wo consecutive duplicate segments will generate
                    // only one duplicate-section.
                    // Actual value is not important as long as it is not -1,0,1
                    assign_loop
                    <
                        int, 0, DimensionCount
                    >::apply(direction_classes, -99);
                }
            }

            if (section.count > 0
                && (!compare_loop
                        <
                            int, 0, DimensionCount
                        >::apply(direction_classes, section.directions)
                    || section.count > MaxCount
                    )
                )
            {
                sections.push_back(section);
                section = sections_range_type();
            }

            if (section.count == 0)
            {
                section.begin_index = i;
                section.ring_index = ring_index;
                section.multi_index = multi_index;
                section.duplicate = duplicate;
                section.non_duplicate_index = ndi;
                section.range_count = boost::size(range);

                copy_loop
                    <
                        int, 0, DimensionCount
                    >::apply(direction_classes, section.directions);
                ggl::combine(section.bounding_box, *previous);
            }

            ggl::combine(section.bounding_box, *it);
            section.end_index = i + 1;
            section.count++;
            if (! duplicate)
            {
                ndi++;
            }
        }

        if (section.count > 0)
        {
            sections.push_back(section);
        }
    }
};

template
<
    typename Polygon,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize_polygon
{
    static inline void apply(Polygon const& poly, Sections& sections,
                int multi_index = -1)
    {
        typedef typename point_type<Polygon>::type point_type;
        typedef typename ring_type<Polygon>::type ring_type;
        typedef sectionalize_range
            <
                ring_type, point_type, Sections, DimensionCount, MaxCount
            > sectionalizer_type;

        typedef typename boost::range_const_iterator
            <
            typename interior_type<Polygon>::type
            >::type iterator_type;

        sectionalizer_type::apply(exterior_ring(poly), sections, -1, multi_index);

        int i = 0;
        for (iterator_type it = boost::begin(interior_rings(poly));
             it != boost::end(interior_rings(poly));
             ++it, ++i)
        {
            sectionalizer_type::apply(*it, sections, i, multi_index);
        }
    }
};

template
<
    typename Box,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize_box
{
    static inline void apply(Box const& box, Sections& sections)
    {
        typedef typename point_type<Box>::type point_type;

        assert_dimension<Box, 2>();

        // Add all four sides of the 2D-box as separate section.
        // Easiest is to convert it to a polygon.
        // However, we don't have the polygon type
        // (or polygon would be a helper-type).
        // Therefore we mimic a linestring/std::vector of 5 points

        point_type ll, lr, ul, ur;
        assign_box_corners(box, ll, lr, ul, ur);

        std::vector<point_type> points;
        points.push_back(ll);
        points.push_back(ul);
        points.push_back(ur);
        points.push_back(lr);
        points.push_back(ll);

        sectionalize_range
            <
                std::vector<point_type>,
                point_type,
                Sections,
                DimensionCount,
                MaxCount
            >::apply(points, sections);
    }
};

}} // namespace detail::sectionalize
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag,
    typename Geometry,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize
{};

template
<
    typename Box,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize<box_tag, Box, Sections, DimensionCount, MaxCount>
    : detail::sectionalize::sectionalize_box
        <
            Box,
            Sections,
            DimensionCount,
            MaxCount
        >
{};

template
<
    typename LineString, typename
    Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize
    <
        linestring_tag,
        LineString,
        Sections,
        DimensionCount,
        MaxCount
    >
    : detail::sectionalize::sectionalize_range
        <
            LineString,
            typename point_type<LineString>::type,
            Sections,
            DimensionCount,
            MaxCount
        >
{};

template
<
    typename Range,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize<ring_tag, Range, Sections, DimensionCount, MaxCount>
    : detail::sectionalize::sectionalize_range
        <
            Range,
            typename point_type<Range>::type,
            Sections,
            DimensionCount,
            MaxCount
        >
{};

template
<
    typename Polygon,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize<polygon_tag, Polygon, Sections, DimensionCount, MaxCount>
    : detail::sectionalize::sectionalize_polygon
        <
            Polygon, Sections, DimensionCount, MaxCount
        >
{};

} // namespace dispatch
#endif


/*!
    \brief Split a geometry into monotonic sections
    \ingroup sectionalize
    \tparam Geometry type of geometry to check
    \tparam Sections type of sections to create
    \param geometry geometry to create sections from
    \param sections structure with sections

 */
template<typename Geometry, typename Sections>
inline void sectionalize(Geometry const& geometry, Sections& sections)
{
    // A maximum of 10 segments per section seems to give the fastest results
    static const std::size_t max_segments_per_section = 10;
    typedef dispatch::sectionalize
        <
            typename tag<Geometry>::type,
            Geometry,
            Sections,
            Sections::value,
            max_segments_per_section
        > sectionalizer_type;

    sections.clear();
    sectionalizer_type::apply(geometry, sections);
}





} // namespace ggl

#endif // GGL_ALGORITHMS_SECTIONALIZE_HPP
