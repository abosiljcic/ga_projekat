#include "watchmanroute.h"

WatchmanRoute::WatchmanRoute(QWidget *pCrtanje,
                             int pauzaKoraka,
                             const bool &naivni,
                             std::string imeDatoteke,
                             int brojTemena)
    : AlgoritamBaza(pCrtanje, pauzaKoraka, naivni)
{
    if (imeDatoteke == "")
        vertices = generisiNasumicneTacke(brojTemena);
    else
        vertices = ucitajPodatkeIzDatoteke(imeDatoteke);
}

void WatchmanRoute::decomposePolygon()
{
    while (vertices.size() != 0) {
            // Step 2: Find the leftmost of the lowest vertices
            auto minIt = std::min_element(vertices.begin(), vertices.end(), [](const QPoint &a, const QPoint &b) {
                if (a.y() == b.y()) return a.x() < b.x();
                return a.y() < b.y();
            });

            QPoint Pk = *minIt;
            vertices.erase(minIt);

            // Find the next leftmost of the lowest vertices
            minIt = std::min_element(vertices.begin(), vertices.end(), [Pk](const QPoint &a, const QPoint &b) {
                if (a.y() == b.y()) return a.x() < b.x();
                return a.y() < b.y();
            });

            QPoint Pl = *minIt;
            vertices.erase(minIt);

            // Step 3: Find the next vertex Pm
            auto rangeIt = std::find_if(vertices.begin(), vertices.end(), [Pk, Pl](const QPoint &v) {
                return v.y() > Pk.y() && v.x() >= Pk.x() && v.x() <= Pl.x();
            });

            if (rangeIt == vertices.end()) {
                // If no vertex found, continue with current Pk and Pl
                rangeIt = std::find_if(vertices.begin(), vertices.end(), [Pk](const QPoint &v) {
                    return v.x() >= Pk.x();
                });
            }

            QPoint Pm = *rangeIt;
            vertices.erase(rangeIt);

            // Step 4: Create the rectangle
            QRect rect(QPoint(Pk.x(), Pk.y()), QSize(Pm.x() - Pk.x(), Pm.y() - Pk.y()));
            rectangles.push_back(rect);

            // Step 5: Remove or add points
            if (std::find(vertices.begin(), vertices.end(), QPointF(Pk.x(), Pm.y())) == vertices.end()) {
                vertices.push_back(QPoint(Pk.x(), Pm.y()));
            }
            if (std::find(vertices.begin(), vertices.end(), QPointF(Pl.x(), Pm.y())) == vertices.end()) {
                vertices.push_back(QPoint(Pl.x(), Pm.y()));
            }
        }
}

void WatchmanRoute::decomposeToBalanced()
{
    if (rectangles.empty()) return;

        // Step 1: Initialize minu and maxl with the first rectangle's upper and lower edges
        int minu = rectangles[0].top(); // top edge is ui
        int maxl = rectangles[0].bottom(); // bottom edge is li

        std::vector<QRect> currentPolygon;
        currentPolygon.push_back(rectangles[0]);

        for (size_t i = 1; i < rectangles.size(); ++i) {
            const QRect& Ri = rectangles[i];
            int ui = Ri.top();
            int li = Ri.bottom();

            // Step 3: Check if this rectangle can be part of the current balanced polygon
            if (ui > maxl || li < minu) {
                // Remove previous rectangles and start a new balanced polygon
                balancedPolygons.insert(balancedPolygons.end(), currentPolygon.begin(), currentPolygon.end());
                currentPolygon.clear();
                currentPolygon.push_back(Ri);
                minu = ui;
                maxl = li;
            } else {
                // Update minu and maxl
                minu = std::min(minu, ui);
                maxl = std::max(maxl, li);
                currentPolygon.push_back(Ri);
            }
        }

        // Add the last balanced sub-polygon
        balancedPolygons.insert(balancedPolygons.end(), currentPolygon.begin(), currentPolygon.end());

}

void WatchmanRoute::trimPath()
{
    if (balancedPolygons.empty()) {
            return;
        }

        // Step 1-2: Iterate from leftmost rectangle to rightmost
        for (size_t i = 0; i < balancedPolygons.size(); ++i) {
            if (balancedPolygons[i].top() > balancedPolygons[0].bottom() || balancedPolygons[i].bottom() < balancedPolygons[0].top()) {
                // Remove rectangles R1 to Ri
                balancedPolygons.erase(balancedPolygons.begin(), balancedPolygons.begin() + i);
                break;
            }
        }

        // Step 3-4: Iterate from rightmost rectangle to leftmost
        for (size_t i = balancedPolygons.size(); i > 0; --i) {
            if (balancedPolygons[i - 1].top() > balancedPolygons.back().bottom() || balancedPolygons[i - 1].bottom() < balancedPolygons.back().top()) {
                // Remove rectangles Ri to Rm
                balancedPolygons.erase(balancedPolygons.begin() + i, balancedPolygons.end());
                break;
            }
        }

        // Step 5: Compute Π = Π ∩ R
        std::vector<QRect> trimmedPath;
        for (const auto& rect : balancedPolygons) {
            trimmedPath.push_back(rect);
        }

        balancedPolygons = trimmedPath;
}
std::vector<int> WatchmanRoute::selectAppropriateAligns()
{
    size_t n = balancedPolygons.size();
    std::vector<int> alignYValues(n);

    // Initialize M and m vectors
    std::vector<int> M(n), m(n);

    // Get the min and max y-values of a rectangle
    auto getMinMaxY = [](const QRect& rect, int& minY, int& maxY) {
        minY = rect.top();
        maxY = rect.bottom();
    };

    // Compute M and m values for each balanced polygon
    for (size_t i = 0; i < n; ++i) {
        int minY, maxY;
        getMinMaxY(balancedPolygons[i], minY, maxY);
        M[i] = minY;
        m[i] = maxY;
    }

    // For first sub-polygon
    alignYValues[0] = (M[0] < M[1]) ? M[0] : m[0];
    for (size_t i = 0; i < n; ++i) {
        // For middle sub-polygons
        if (M[i - 1] < M[i] && M[i] > M[i + 1]) {
            alignYValues[i] = m[i];
        }
        else {
            alignYValues[i] = M[i];
        }
    }

    // For last sub-polygon
    alignYValues[n-1] = (M[n-1] < M[n - 2]) ? M[n-1] : m[n-1];

    return alignYValues;
}

void WatchmanRoute::createFinalRoute()
{
    std::vector<int> alignYValues = selectAppropriateAligns();

    size_t n = balancedPolygons.size();
    if (n == 0 || alignYValues.size() != n) return;

    // Create align segments
    std::vector<Segment> alignSegments;
    for (size_t i = 0; i < n; ++i) {
        int x1 = balancedPolygons[i].left();
        int x2 = balancedPolygons[i].right();
        int y = alignYValues[i];
        alignSegments.emplace_back(x1, y, x2, y);
    }

    // Create the route by connecting align segments
    for (size_t i = 0; i < n - 1; ++i) {
        // Add horizontal segment for align[i]
        route.push_back(alignSegments[i]);

        // Add vertical segment connecting align[i] to align[i+1]
        int x1 = std::get<2>(alignSegments[i]); // right x of current segment
        int y1 = std::get<3>(alignSegments[i]); // y of current segment
        int x2 = std::get<0>(alignSegments[i+1]); // left x of next segment
        int y2 = std::get<1>(alignSegments[i+1]); // y of next segment
        route.emplace_back(x1, y1, x2, y2);
    }

    // Add the last align segment
    route.push_back(alignSegments.back());
}

void WatchmanRoute::pokreniAlgoritam()
{
    decomposePolygon();
    AlgoritamBaza_updateCanvasAndBlock();

    decomposeToBalanced();

    AlgoritamBaza_updateCanvasAndBlock();
    trimPath();

    AlgoritamBaza_updateCanvasAndBlock();
    alignSegmentsY = selectAppropriateAligns();

    AlgoritamBaza_updateCanvasAndBlock();

    createFinalRoute();
    AlgoritamBaza_updateCanvasAndBlock();

    emit animacijaZavrsila();
}

void WatchmanRoute::drawVertices(QPainter *painter) const
{
    if (!painter)
        return;

    QPen pen(Qt::black);
    pen.setWidth(5);
    painter->setPen(pen);
    for (const QPoint &vertex : vertices) {
        painter->drawPoint(vertex);
    }
}

void WatchmanRoute::drawRectangles(QPainter *painter) const
{
    if (!painter)
        return;

    QPen pen(Qt::blue);
    pen.setWidth(2);
    painter->setPen(pen);
    for (const QRect &rect : rectangles) {
        painter->drawRect(rect);
    }
}

void WatchmanRoute::drawBalancedPolygons(QPainter* painter) const
{
        if (!painter)
            return;

        QPen pen(Qt::green);
        pen.setWidth(2);
        painter->setPen(pen);

        for (const QRect &rect : balancedPolygons) {
            painter->drawRect(rect);
        }
    }

void WatchmanRoute::drawRoute(QPainter* painter) const
{
    QPen pen(Qt::red);
    pen.setWidth(1);
    painter->setPen(pen);

    for (const auto& segment : route)
    {
        int x1, y1, x2, y2;
        std::tie(x1, y1, x2, y2) = segment;
        painter->drawLine(x1, y1, x2, y2);
    }
}

void WatchmanRoute::drawAlignSegment(QPainter* painter) const
{
    QPen pen(Qt::magenta);
    pen.setWidth(2);
    painter->setPen(pen);

    for (int y : alignSegmentsY)
    {
        painter->drawLine(0, y, _pCrtanje->width(), y);
    }
}


void WatchmanRoute::crtajAlgoritam(QPainter *painter) const
{
        if (!painter)
            return;

        drawVertices(painter);
        drawRectangles(painter);
        drawBalancedPolygons(painter);
        drawAlignSegment(painter);
        drawRoute(painter);
}

void WatchmanRoute::pokreniNaivniAlgoritam()
{

}

void WatchmanRoute::crtajNaivniAlgoritam(QPainter *painter) const
{

}
