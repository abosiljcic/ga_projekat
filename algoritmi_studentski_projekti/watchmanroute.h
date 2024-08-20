#ifndef WATCHMANROUTE_H
#define WATCHMANROUTE_H

#include "algoritambaza.h"
#include <QPoint>
#include <QRect>
#include <vector>
#include<iostream>

using Segment = std::tuple<int, int, int, int>;

class WatchmanRoute : public AlgoritamBaza{
public:
    WatchmanRoute(QWidget *pCrtanje,
                  int pauzaKoraka,
                  const bool &naivni = false,
                  std::string imeDatoteke = "",
                  int brojPoligona = BROJ_SLUCAJNIH_OBJEKATA);

private:
    void pokreniAlgoritam() final;
    void crtajAlgoritam(QPainter *painter) const final;
    void pokreniNaivniAlgoritam() final;
    void crtajNaivniAlgoritam(QPainter *painter) const final;

    void drawVertices(QPainter *painter) const;
    void drawRectangles(QPainter *painter) const;
    void drawBalancedPolygons(QPainter* painter) const;
    void drawRoute(QPainter* painter) const;
    void drawAlignSegment(QPainter* painter) const;

    void decomposePolygon();
    void decomposeToBalanced();
    void trimPath();
    std::vector<int> selectAppropriateAligns();
    void createFinalRoute();

    std::vector<QPoint> vertices;
    std::vector<QRect> rectangles;
    std::vector<QRect> balancedPolygons;
    std::vector<int> alignSegmentsY;
    std::vector<Segment> route;
};

#endif // WATCHMANROUTE_H
