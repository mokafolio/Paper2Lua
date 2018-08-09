#ifndef PAPERLUA_PAPER2LUA_HPP
#define PAPERLUA_PAPER2LUA_HPP

#include <Luanatic/Luanatic.hpp>
#include <Paper2/Document.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Tarp/TarpRenderer.hpp>
#include <Stick/Path.hpp>

namespace luanatic
{
    template<>
    struct ValueTypeConverter<paper::ColorStop>
    {
        static paper::ColorStop convertAndCheck(lua_State * _state, stick::Int32 _index)
        {
            return paper::ColorStop{};
        }

        static stick::Int32 push(lua_State * _state, const paper::ColorStop & _stop)
        {
            lua_createtable(_state, 2, 0);
            luanatic::pushValueType(_state, _stop.color);
            lua_setfield(_state, -2, "color");
            lua_pushnumber(_state, _stop.offset);
            lua_setfield(_state, -2, "offset");
            return 1;
        }
    };
}

namespace paperLua
{
    STICK_API inline void registerPaper(lua_State * _state, const stick::String & _namespace = "");

    namespace detail
    {
        using namespace paper;

        inline Int32 luaClosestCurveLocation(lua_State * _state)
        {
            Path * p = luanatic::convertToTypeAndCheck<Path>(_state, 1);
            Float dist;
            auto cl = p->closestCurveLocation(luanatic::detail::Converter<const Vec2f &>::convert(_state, 2), dist);
            luanatic::pushValueType<CurveLocation>(_state, cl);
            lua_pushnumber(_state, dist);
            return 2;
        }

        inline Int32 luaIntersections(lua_State * _state)
        {
            Path * a = luanatic::convertToTypeAndCheck<Path>(_state, 1);
            Path * b = luanatic::convertToTypeAndCheck<Path>(_state, 2);
            auto inter = a->intersections(b);
            if (inter.count())
            {
                lua_createtable(_state, inter.count(), 0);
                for (stick::Int32 i = 0; i < inter.count(); ++i)
                {
                    lua_pushinteger(_state, i + 1);
                    lua_newtable(_state);
                    luanatic::pushValueType<CurveLocation>(_state, inter[i].location);
                    lua_setfield(_state, -2, "location");
                    luanatic::pushValueType<Vec2f>(_state, inter[i].position);
                    lua_setfield(_state, -2, "position");
                    lua_settable(_state, -3);
                }
            }
            else
                lua_pushnil(_state);
            return 1;
        }


        template<class CV>
        struct LuaContainerViewSession
        {
            CV view;
            typename CV::ValueType tmp;
            typename CV::Iter iter;
        };

        template<class CV>
        inline Int32 luaContainerViewIteratorFunction(lua_State * _state)
        {
            STICK_ASSERT(lua_isuserdata(_state, lua_upvalueindex(1)));
            LuaContainerViewSession<CV> * session = static_cast<LuaContainerViewSession<CV> *>(lua_touserdata(_state, lua_upvalueindex(1)));
            STICK_ASSERT(session);
            if (session->iter != session->view.end())
            {
                session->tmp = *session->iter++;
                luanatic::push<typename CV::ValueType>(_state, &session->tmp, false);
            }
            else
            {
                //we reached the end
                lua_pushnil(_state);
                stick::defaultAllocator().destroy(session);
            }

            return 1;
        }

        template<class CV, CV(Path::*Getter)()>
        inline Int32 luaPushContainerView(lua_State * _state)
        {
            Path * path = luanatic::convertToTypeAndCheck<Path>(_state, 1);
            LuaContainerViewSession<CV> * session = stick::defaultAllocator().create<LuaContainerViewSession<CV>>();
            session->view = (path->*Getter)();
            session->iter = session->view.begin();
            lua_pushlightuserdata(_state, session);
            lua_pushcclosure(_state, luaContainerViewIteratorFunction<CV>, 1);
            return 1;
        }
    }

    inline void registerPaper(lua_State * _state, const stick::String & _namespace)
    {
        using namespace luanatic;
        using namespace paper;
        using namespace stick;

        LuaValue g = globalsTable(_state);
        LuaValue namespaceTable = g;
        if (!_namespace.isEmpty())
        {
            auto tokens = path::segments(_namespace, '.');
            for (const String & token : tokens)
            {
                LuaValue table = namespaceTable.findOrCreateTable(token);
                namespaceTable = table;
            }
        }

        //register constants
        LuaValue strokeJoinTable = namespaceTable.findOrCreateTable("StrokeJoin");
        strokeJoinTable["Miter"].set(StrokeJoin::Miter);
        strokeJoinTable["Round"].set(StrokeJoin::Round);
        strokeJoinTable["Bevel"].set(StrokeJoin::Bevel);

        LuaValue strokeCapTable = namespaceTable.findOrCreateTable("StrokeCap");
        strokeCapTable["Round"].set(StrokeCap::Round);
        strokeCapTable["Square"].set(StrokeCap::Square);
        strokeCapTable["Butt"].set(StrokeCap::Butt);

        LuaValue windingRuleTable = namespaceTable.findOrCreateTable("WindingRule");
        windingRuleTable["EvenOdd"].set(WindingRule::EvenOdd);
        windingRuleTable["NonZero"].set(WindingRule::NonZero);

        LuaValue smoothingTypeTable = namespaceTable.findOrCreateTable("Smoothing");
        smoothingTypeTable["Continuous"].set(Smoothing::Continuous);
        smoothingTypeTable["Asymmetric"].set(Smoothing::Asymmetric);
        smoothingTypeTable["CatmullRom"].set(Smoothing::CatmullRom);
        smoothingTypeTable["Geometric"].set(Smoothing::Geometric);

        LuaValue itemTypeTable = namespaceTable.findOrCreateTable("ItemType");
        itemTypeTable["Document"].set(ItemType::Document);
        itemTypeTable["Group"].set(ItemType::Group);
        itemTypeTable["Path"].set(ItemType::Path);
        itemTypeTable["Symbol"].set(ItemType::Symbol);
        itemTypeTable["Unknown"].set(ItemType::Unknown);

        ClassWrapper<Segment> segmentCW("Segment");
        segmentCW.
        addMemberFunction("setPosition", LUANATIC_FUNCTION(&Segment::setPosition)).
        addMemberFunction("setHandleIn", LUANATIC_FUNCTION(&Segment::setHandleIn)).
        addMemberFunction("setHandleOut", LUANATIC_FUNCTION(&Segment::setHandleOut)).
        addMemberFunction("position", LUANATIC_FUNCTION(&Segment::position)).
        addMemberFunction("handleIn", LUANATIC_FUNCTION(&Segment::handleIn)).
        addMemberFunction("handleOut", LUANATIC_FUNCTION(&Segment::handleOut)).
        addMemberFunction("handleInAbsolute", LUANATIC_FUNCTION(&Segment::handleInAbsolute)).
        addMemberFunction("handleOutAbsolute", LUANATIC_FUNCTION(&Segment::handleOutAbsolute)).
        addMemberFunction("isLinear", LUANATIC_FUNCTION(&Segment::isLinear)).
        addMemberFunction("remove", LUANATIC_FUNCTION(&Segment::remove));

        namespaceTable.registerClass(segmentCW);

        ClassWrapper<CurveLocation> curveLocationCW("CurveLocation");
        curveLocationCW.
        addMemberFunction("position", LUANATIC_FUNCTION(&CurveLocation::position)).
        addMemberFunction("normal", LUANATIC_FUNCTION(&CurveLocation::normal)).
        addMemberFunction("tangent", LUANATIC_FUNCTION(&CurveLocation::tangent)).
        addMemberFunction("curvature", LUANATIC_FUNCTION(&CurveLocation::curvature)).
        addMemberFunction("angle", LUANATIC_FUNCTION(&CurveLocation::angle)).
        addMemberFunction("parameter", LUANATIC_FUNCTION(&CurveLocation::parameter)).
        addMemberFunction("offset", LUANATIC_FUNCTION(&CurveLocation::offset)).
        addMemberFunction("isValid", LUANATIC_FUNCTION(&CurveLocation::isValid)).
        addMemberFunction("curve", LUANATIC_FUNCTION(&CurveLocation::curve));

        namespaceTable.registerClass(curveLocationCW);

        ClassWrapper<Curve> curveCW("Curve");
        curveCW.
        addMemberFunction("path", LUANATIC_FUNCTION(&Curve::path)).
        addMemberFunction("setPositionOne", LUANATIC_FUNCTION(&Curve::setPositionOne)).
        addMemberFunction("setHandleOne", LUANATIC_FUNCTION(&Curve::setHandleOne)).
        addMemberFunction("setPositionTwo", LUANATIC_FUNCTION(&Curve::setPositionTwo)).
        addMemberFunction("setHandleTwo", LUANATIC_FUNCTION(&Curve::setHandleTwo)).
        addMemberFunction("positionOne", LUANATIC_FUNCTION(&Curve::positionOne)).
        addMemberFunction("positionTwo", LUANATIC_FUNCTION(&Curve::positionTwo)).
        addMemberFunction("handleOne", LUANATIC_FUNCTION(&Curve::handleOne)).
        addMemberFunction("handleOneAbsolute", LUANATIC_FUNCTION(&Curve::handleOneAbsolute)).
        addMemberFunction("handleTwo", LUANATIC_FUNCTION(&Curve::handleTwo)).
        addMemberFunction("handleTwoAbsolute", LUANATIC_FUNCTION(&Curve::handleTwoAbsolute)).
        addMemberFunction("positionAt", LUANATIC_FUNCTION(&Curve::positionAt)).
        addMemberFunction("normalAt", LUANATIC_FUNCTION(&Curve::normalAt)).
        addMemberFunction("tangentAt", LUANATIC_FUNCTION(&Curve::tangentAt)).
        addMemberFunction("curvatureAt", LUANATIC_FUNCTION(&Curve::curvatureAt)).
        addMemberFunction("angleAt", LUANATIC_FUNCTION(&Curve::angleAt)).
        addMemberFunction("positionAtParameter", LUANATIC_FUNCTION(&Curve::positionAtParameter)).
        addMemberFunction("normalAtParameter", LUANATIC_FUNCTION(&Curve::normalAtParameter)).
        addMemberFunction("tangentAtParameter", LUANATIC_FUNCTION(&Curve::tangentAtParameter)).
        addMemberFunction("curvatureAtParameter", LUANATIC_FUNCTION(&Curve::curvatureAtParameter)).
        addMemberFunction("angleAtParameter", LUANATIC_FUNCTION(&Curve::angleAtParameter)).
        addMemberFunction("parameterAtOffset", LUANATIC_FUNCTION(&Curve::parameterAtOffset)).
        addMemberFunction("closestParameter", LUANATIC_FUNCTION_OVERLOAD(Float(Curve::*)(const Vec2f &)const, &Curve::closestParameter)).
        addMemberFunction("lengthBetween", LUANATIC_FUNCTION(&Curve::lengthBetween)).
        addMemberFunction("pathOffset", LUANATIC_FUNCTION(&Curve::pathOffset)).
        addMemberFunction("closestCurveLocation", LUANATIC_FUNCTION(&Curve::closestCurveLocation)).
        addMemberFunction("curveLocationAt", LUANATIC_FUNCTION(&Curve::curveLocationAt)).
        addMemberFunction("curveLocationAtParameter", LUANATIC_FUNCTION(&Curve::curveLocationAtParameter)).
        addMemberFunction("isLinear", LUANATIC_FUNCTION(&Curve::isLinear)).
        addMemberFunction("isStraight", LUANATIC_FUNCTION(&Curve::isStraight)).
        addMemberFunction("isArc", LUANATIC_FUNCTION(&Curve::isArc)).
        addMemberFunction("isOrthogonal", LUANATIC_FUNCTION(&Curve::isOrthogonal)).
        addMemberFunction("isCollinear", LUANATIC_FUNCTION(&Curve::isCollinear)).
        addMemberFunction("length", LUANATIC_FUNCTION(&Curve::length)).
        addMemberFunction("area", LUANATIC_FUNCTION(&Curve::area)).
        addMemberFunction("divideAt", LUANATIC_FUNCTION(&Curve::divideAt)).
        addMemberFunction("divideAtParameter", LUANATIC_FUNCTION(&Curve::divideAtParameter)).
        addMemberFunction("bounds", LUANATIC_FUNCTION_OVERLOAD(const Rect & (Curve::*)()const, &Curve::bounds)).
        addMemberFunction("boundsWithPadding", LUANATIC_FUNCTION_OVERLOAD(Rect(Curve::*)(Float)const, &Curve::bounds));

        namespaceTable.registerClass(curveCW);

        ClassWrapper<NoPaint> noPaintCW("NoPaint");
        noPaintCW.
        addConstructor<>();

        namespaceTable.registerClass(noPaintCW);

        ClassWrapper<BaseGradient> baseGradientCW("BaseGradient");
        baseGradientCW.
        addMemberFunction("setOrigin", LUANATIC_FUNCTION(&BaseGradient::setOrigin)).
        addMemberFunction("setDestination", LUANATIC_FUNCTION(&BaseGradient::setDestination)).
        addMemberFunction("addStop", LUANATIC_FUNCTION(&BaseGradient::addStop)).
        addMemberFunction("origin", LUANATIC_FUNCTION(&BaseGradient::origin)).
        addMemberFunction("destination", LUANATIC_FUNCTION(&BaseGradient::destination)).
        addMemberFunction("stops", LUANATIC_FUNCTION(&BaseGradient::stops, ReturnIterator<ph::Result>));

        namespaceTable.registerClass(baseGradientCW);

        ClassWrapper<LinearGradient> linearGradientCW("LinearGradient");
        linearGradientCW.
        addBase<BaseGradient>();

        namespaceTable.
        registerClass(linearGradientCW).
        addWrapper<LinearGradient, LinearGradientPtr>();

        ClassWrapper<RadialGradient> radialGradientCW("RadialGradient");
        radialGradientCW.
        addBase<BaseGradient>().
        addMemberFunction("setFocalPointOffset", LUANATIC_FUNCTION(&RadialGradient::setFocalPointOffset)).
        addMemberFunction("setRatio", LUANATIC_FUNCTION(&RadialGradient::setRatio)).
        addMemberFunction("focalPointOffset", LUANATIC_FUNCTION(&RadialGradient::focalPointOffset)).
        addMemberFunction("ratio", LUANATIC_FUNCTION(&RadialGradient::ratio));

        namespaceTable.
        registerClass(radialGradientCW).
        addWrapper<RadialGradient, RadialGradientPtr>();

        ClassWrapper<Item> itemCW("Item");
        itemCW.
        addMemberFunction("addChild", LUANATIC_FUNCTION(&Item::addChild)).
        addMemberFunction("insertAbove", LUANATIC_FUNCTION(&Item::insertAbove)).
        addMemberFunction("insertBelow", LUANATIC_FUNCTION(&Item::insertBelow)).
        addMemberFunction("sendToFront", LUANATIC_FUNCTION(&Item::sendToFront)).
        addMemberFunction("sendToBack", LUANATIC_FUNCTION(&Item::sendToBack)).
        addMemberFunction("reverseChildren", LUANATIC_FUNCTION(&Item::reverseChildren)).
        addMemberFunction("remove", LUANATIC_FUNCTION(&Item::remove)).
        addMemberFunction("removeChildren", LUANATIC_FUNCTION(&Item::removeChildren)).
        addMemberFunction("name", LUANATIC_FUNCTION(&Item::name)).
        addMemberFunction("parent", LUANATIC_FUNCTION(&Item::parent)).
        addMemberFunction("setPosition", LUANATIC_FUNCTION(&Item::setPosition)).
        addMemberFunction("setPivot", LUANATIC_FUNCTION(&Item::setPivot)).
        addMemberFunction("setVisible", LUANATIC_FUNCTION(&Item::setVisible)).
        addMemberFunction("setName", LUANATIC_FUNCTION(&Item::setName)).
        addMemberFunction("setTransform", LUANATIC_FUNCTION(&Item::setTransform)).
        addMemberFunction("translateTransform", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const Vec2f &), &Item::translateTransform)).
        addMemberFunction("scaleTransform", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const Vec2f &), &Item::scaleTransform)).
        addMemberFunction("scaleAroundTransform", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const Vec2f &, const Vec2f &), &Item::scaleTransform)).
        addMemberFunction("rotateTransform", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(Float), &Item::rotateTransform)).
        addMemberFunction("rotateAroundTransform", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(Float, const Vec2f &), &Item::rotateTransform)).
        addMemberFunction("transformItem", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const Mat32f &), &Item::transform)).
        addMemberFunction("translate", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const Vec2f &), &Item::translate)).
        addMemberFunction("scale", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const Vec2f &), &Item::scale)).
        addMemberFunction("rotate", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(Float), &Item::rotate)).
        addMemberFunction("rotateAround", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(Float, const Vec2f &), &Item::rotate)).
        addMemberFunction("applyTransform", LUANATIC_FUNCTION(&Item::applyTransform)).
        addMemberFunction("transform", LUANATIC_FUNCTION_OVERLOAD(const Mat32f & (Item::*)()const, &Item::transform)).
        addMemberFunction("rotation", LUANATIC_FUNCTION(&Item::rotation)).
        addMemberFunction("translation", LUANATIC_FUNCTION(&Item::translation)).
        addMemberFunction("scaling", LUANATIC_FUNCTION(&Item::scaling)).
        addMemberFunction("absoluteRotation", LUANATIC_FUNCTION(&Item::absoluteRotation)).
        addMemberFunction("absoluteTranslation", LUANATIC_FUNCTION(&Item::absoluteTranslation)).
        addMemberFunction("absoluteScaling", LUANATIC_FUNCTION(&Item::absoluteScaling)).
        addMemberFunction("bounds", LUANATIC_FUNCTION(&Item::bounds)).
        addMemberFunction("handleBounds", LUANATIC_FUNCTION(&Item::handleBounds)).
        addMemberFunction("strokeBounds", LUANATIC_FUNCTION(&Item::strokeBounds)).
        addMemberFunction("position", LUANATIC_FUNCTION(&Item::position)).
        addMemberFunction("pivot", LUANATIC_FUNCTION(&Item::pivot)).
        addMemberFunction("isVisible", LUANATIC_FUNCTION(&Item::isVisible)).
        addMemberFunction("setStrokeJoin", LUANATIC_FUNCTION(&Item::setStrokeJoin)).
        addMemberFunction("setStrokeCap", LUANATIC_FUNCTION(&Item::setStrokeCap)).
        addMemberFunction("setMiterLimit", LUANATIC_FUNCTION(&Item::setMiterLimit)).
        addMemberFunction("setStrokeWidth", LUANATIC_FUNCTION(&Item::setStrokeWidth)).
        addMemberFunction("setDashArray", LUANATIC_FUNCTION(&Item::setDashArray)).
        addMemberFunction("setDashOffset", LUANATIC_FUNCTION(&Item::setDashOffset)).
        addMemberFunction("setScaleStroke", LUANATIC_FUNCTION(&Item::setScaleStroke)).
        addMemberFunction("setStrokeColor", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const ColorRGBA &), &Item::setStroke)).
        addMemberFunction("setStrokeString", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const stick::String &), &Item::setStroke)).
        addMemberFunction("setStrokeLinearGradient", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const LinearGradientPtr &), &Item::setStroke)).
        addMemberFunction("setStrokeRadialGradient", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const RadialGradientPtr &), &Item::setStroke)).
        addMemberFunction("setStroke", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const ColorRGBA &), &Item::setStroke)).
        addMemberFunction("setStroke", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const stick::String &), &Item::setStroke)).
        addMemberFunction("setStroke", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const LinearGradientPtr &), &Item::setStroke)).
        addMemberFunction("setStroke", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const RadialGradientPtr &), &Item::setStroke)).
        addMemberFunction("removeStroke", LUANATIC_FUNCTION(&Item::removeStroke)).
        addMemberFunction("setFillColor", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const ColorRGBA &), &Item::setFill)).
        addMemberFunction("setFillString", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const stick::String &), &Item::setFill)).
        addMemberFunction("setFillLinearGradient", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const LinearGradientPtr &), &Item::setFill)).
        addMemberFunction("setFillRadialGradient", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const RadialGradientPtr &), &Item::setFill)).
        addMemberFunction("setFill", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const ColorRGBA &), &Item::setFill)).
        addMemberFunction("setFill", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const stick::String &), &Item::setFill)).
        addMemberFunction("setFill", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const LinearGradientPtr &), &Item::setFill)).
        addMemberFunction("setFill", LUANATIC_FUNCTION_OVERLOAD(void(Item::*)(const RadialGradientPtr &), &Item::setFill)).
        addMemberFunction("removeFill", LUANATIC_FUNCTION(&Item::removeFill)).
        addMemberFunction("setWindingRule", LUANATIC_FUNCTION(&Item::setWindingRule)).
        addMemberFunction("strokeJoin", LUANATIC_FUNCTION(&Item::strokeJoin)).
        addMemberFunction("strokeCap", LUANATIC_FUNCTION(&Item::strokeCap)).
        addMemberFunction("miterLimit", LUANATIC_FUNCTION(&Item::miterLimit)).
        addMemberFunction("strokeWidth", LUANATIC_FUNCTION(&Item::strokeWidth)).
        addMemberFunction("dashArray", LUANATIC_FUNCTION(&Item::dashArray)).
        addMemberFunction("dashOffset", LUANATIC_FUNCTION(&Item::dashOffset)).
        addMemberFunction("windingRule", LUANATIC_FUNCTION(&Item::windingRule)).
        addMemberFunction("scaleStroke", LUANATIC_FUNCTION(&Item::scaleStroke)).
        addMemberFunction("fill", LUANATIC_FUNCTION(&Item::fill)).
        addMemberFunction("stroke", LUANATIC_FUNCTION(&Item::stroke)).
        addMemberFunction("hasStroke", LUANATIC_FUNCTION(&Item::hasStroke)).
        addMemberFunction("hasFill", LUANATIC_FUNCTION(&Item::hasFill)).
        addMemberFunction("clone", LUANATIC_FUNCTION(&Item::clone)).
        addMemberFunction("document", LUANATIC_FUNCTION(&Item::document)).
        addMemberFunction("itemType", LUANATIC_FUNCTION(&Item::itemType)).
        addMemberFunction("children", LUANATIC_FUNCTION(&Item::children, ReturnIterator<ph::Result>)).
        addMemberFunction("exportSVG", LUANATIC_FUNCTION(&Item::exportSVG)).
        addMemberFunction("saveSVG", LUANATIC_FUNCTION(&Item::saveSVG));

        namespaceTable.registerClass(itemCW);

        ClassWrapper<Group> groupCW("Group");
        groupCW.
        addBase<Item>().
        addMemberFunction("setClipped", LUANATIC_FUNCTION(&Group::setClipped)).
        addMemberFunction("isClipped", LUANATIC_FUNCTION(&Group::isClipped));

        namespaceTable.registerClass(groupCW);

        ClassWrapper<Path> pathCW("Path");
        pathCW.
        addBase<Item>().
        addMemberFunction("addPoint", LUANATIC_FUNCTION(&Path::addPoint)).
        addMemberFunction("cubicCurveTo", LUANATIC_FUNCTION(&Path::cubicCurveTo)).
        addMemberFunction("quadraticCurveTo", LUANATIC_FUNCTION(&Path::quadraticCurveTo)).
        addMemberFunction("curveTo", LUANATIC_FUNCTION(&Path::curveTo)).
        addMemberFunction("arcThrough", LUANATIC_FUNCTION_OVERLOAD(Error(Path::*)(const Vec2f &, const Vec2f &), &Path::arcTo)).
        addMemberFunction("arcTo", LUANATIC_FUNCTION_OVERLOAD(Error(Path::*)(const Vec2f &, bool), &Path::arcTo)).
        addMemberFunction("cubicCurveBy", LUANATIC_FUNCTION(&Path::cubicCurveBy)).
        addMemberFunction("quadraticCurveBy", LUANATIC_FUNCTION(&Path::quadraticCurveBy)).
        addMemberFunction("curveBy", LUANATIC_FUNCTION(&Path::curveBy)).
        addMemberFunction("closePath", LUANATIC_FUNCTION(&Path::closePath)).
        addMemberFunction("smooth", LUANATIC_FUNCTION_OVERLOAD(void(Path::*)(Smoothing, bool), &Path::smooth)).
        addMemberFunction("smoothFromTo", LUANATIC_FUNCTION_OVERLOAD(void(Path::*)(Int64, Int64, Smoothing), &Path::smooth)).
        addMemberFunction("simplify", LUANATIC_FUNCTION(&Path::simplify)).
        addMemberFunction("addSegment", LUANATIC_FUNCTION(&Path::addSegment)).
        addMemberFunction("removeSegment", LUANATIC_FUNCTION(&Path::removeSegment)).
        addMemberFunction("removeSegmentsFrom", LUANATIC_FUNCTION_OVERLOAD(void(Path::*)(Size), &Path::removeSegments)).
        addMemberFunction("removeSegmentsFromTo", LUANATIC_FUNCTION_OVERLOAD(void(Path::*)(Size, Size), &Path::removeSegments)).
        addMemberFunction("removeSegments", LUANATIC_FUNCTION_OVERLOAD(void(Path::*)(), &Path::removeSegments)).
        addMemberFunction("segments", detail::luaPushContainerView<SegmentView, &Path::segments>).
        addMemberFunction("curves", detail::luaPushContainerView<CurveView, &Path::curves>).
        addMemberFunction("positionAt", LUANATIC_FUNCTION(&Path::positionAt)).
        addMemberFunction("normalAt", LUANATIC_FUNCTION(&Path::normalAt)).
        addMemberFunction("tangentAt", LUANATIC_FUNCTION(&Path::tangentAt)).
        addMemberFunction("curvatureAt", LUANATIC_FUNCTION(&Path::curvatureAt)).
        addMemberFunction("angleAt", LUANATIC_FUNCTION(&Path::angleAt)).
        addMemberFunction("reverse", LUANATIC_FUNCTION(&Path::reverse)).
        addMemberFunction("setClockwise", LUANATIC_FUNCTION(&Path::setClockwise)).
        addMemberFunction("flatten", LUANATIC_FUNCTION(&Path::flatten)).
        addMemberFunction("flattenRegular", LUANATIC_FUNCTION(&Path::flattenRegular)).
        addMemberFunction("regularOffset", LUANATIC_FUNCTION(&Path::regularOffset)).
        addMemberFunction("closestCurveLocation", &detail::luaClosestCurveLocation).
        addMemberFunction("curveLocationAt", LUANATIC_FUNCTION(&Path::curveLocationAt)).
        addMemberFunction("length", LUANATIC_FUNCTION(&Path::length)).
        addMemberFunction("area", LUANATIC_FUNCTION(&Path::area)).
        addMemberFunction("extrema", LUANATIC_FUNCTION_OVERLOAD(stick::DynamicArray<CurveLocation> (Path::*)()const, &Path::extrema)).
        addMemberFunction("isClosed", LUANATIC_FUNCTION(&Path::isClosed)).
        addMemberFunction("isClockwise", LUANATIC_FUNCTION(&Path::isClockwise)).
        addMemberFunction("contains", LUANATIC_FUNCTION(&Path::contains)).
        addMemberFunction("segment", LUANATIC_FUNCTION_OVERLOAD(Segment (Path::*)(stick::Size), &Path::segment)).
        addMemberFunction("curve", LUANATIC_FUNCTION_OVERLOAD(Curve (Path::*)(stick::Size), &Path::curve)).
        addMemberFunction("segmentCount", LUANATIC_FUNCTION(&Path::segmentCount)).
        addMemberFunction("curveCount", LUANATIC_FUNCTION(&Path::curveCount)).
        addMemberFunction("intersections", detail::luaIntersections).
        addMemberFunction("slice", LUANATIC_FUNCTION_OVERLOAD(Path * (Path::*)(CurveLocation, CurveLocation)const, &Path::slice)).
        addMemberFunction("slice", LUANATIC_FUNCTION_OVERLOAD(Path * (Path::*)(Float, Float)const, &Path::slice));

        namespaceTable.registerClass(pathCW);

        ClassWrapper<Document> docCW("Document");
        docCW.
        addBase<Item>().
        addConstructor<>().
        addConstructor<const char *>().
        addConstructor("new").
        addConstructor<const char *>("newWithName").
        addMemberFunction("createGroup", LUANATIC_FUNCTION(&Document::createGroup), "").
        addMemberFunction("createPath", LUANATIC_FUNCTION(&Document::createPath), "").
        addMemberFunction("createEllipse", LUANATIC_FUNCTION(&Document::createEllipse), "").
        addMemberFunction("createCircle", LUANATIC_FUNCTION(&Document::createCircle), "").
        addMemberFunction("createRectangle", LUANATIC_FUNCTION(&Document::createRectangle), "").
        addMemberFunction("createRoundedRectangle", LUANATIC_FUNCTION(&Document::createRoundedRectangle), "").
        addMemberFunction("setSize", LUANATIC_FUNCTION(&Document::setSize)).
        addMemberFunction("width", LUANATIC_FUNCTION(&Document::width)).
        addMemberFunction("height", LUANATIC_FUNCTION(&Document::height)).
        addMemberFunction("size", LUANATIC_FUNCTION(&Document::size));
        // addMemberFunction("parseSVG", detail::luaParseSVG).
        // addMemberFunction("loadSVG", detail::luaLoadSVG);

        namespaceTable.
        registerClass(docCW).
        registerFunction("createLinearGradient", LUANATIC_FUNCTION(&createLinearGradient)).
        registerFunction("createRadialGradient", LUANATIC_FUNCTION(&createRadialGradient));

        ClassWrapper<RenderInterface> rendererCW("RenderInterface");
        rendererCW.
        addMemberFunction("setViewport", LUANATIC_FUNCTION(&RenderInterface::setViewport)).
        addMemberFunction("setProjection", LUANATIC_FUNCTION(&RenderInterface::setProjection)).
        addMemberFunction("document", LUANATIC_FUNCTION(&RenderInterface::document)).
        addMemberFunction("draw", LUANATIC_FUNCTION(&RenderInterface::draw));

        namespaceTable.registerClass(rendererCW);

        //TODO allow to specify which renderer to register via macro or template or possibly put
        //it into a separate function?
        ClassWrapper<tarp::TarpRenderer> tarpRendererCW("TarpRenderer");
        tarpRendererCW.
        addBase<RenderInterface>().
        addConstructor<>().
        addMemberFunction("init", LUANATIC_FUNCTION(&tarp::TarpRenderer::init));

        namespaceTable.registerClass(tarpRendererCW);
    }
}

#endif //PAPERLUA_PAPERLUA_HPP
