#include "GPSLine.h"

void GPSLine::calculatePath(CVector destPosn, short& nodesCount, CNodeAddress* resultNodes, CVector2D* nodePoints, float& gpsDistance) {
    destPosn.z = CWorld::FindGroundZForCoord(destPosn.x, destPosn.y);

    ThePaths.DoPathSearch(0, FindPlayerCoors(0), CNodeAddress(), destPosn, resultNodes, &nodesCount, MAX_NODE_POINTS, &gpsDistance,
        999999.0f, NULL, 999999.0f, false, CNodeAddress(), false, FindPlayerPed(0)->m_pVehicle->m_nVehicleSubClass == VEHICLE_BOAT);

    if (nodesCount > 0) {
        for (short i = 0; i < nodesCount; i++) {
            CVector nodePosn = ThePaths.GetPathNode(resultNodes[i])->GetNodeCoors();
            CVector2D tmpPoint;
            CRadar::TransformRealWorldPointToRadarSpace(tmpPoint, CVector2D(nodePosn.x, nodePosn.y));
            if (!FrontEndMenuManager.m_bDrawRadarOrMap)
                CRadar::TransformRadarPointToScreenSpace(nodePoints[i], tmpPoint);
            else {
                CRadar::LimitRadarPoint(tmpPoint);
                CRadar::TransformRadarPointToScreenSpace(nodePoints[i], tmpPoint);
                nodePoints[i].x *= static_cast<float>(RsGlobal.maximumWidth) / 640.0f;
                nodePoints[i].y *= static_cast<float>(RsGlobal.maximumHeight) / 448.0f;
                CRadar::LimitToMap(&nodePoints[i].x, &nodePoints[i].y);
            }
        }
    }
}

void GPSLine::Setup2dVertex(RwIm2DVertex& vertex, float x, float y, short color, bool friendly) {
    vertex.x = x;
    vertex.y = y;
    vertex.u = vertex.v = 0.0f;
    vertex.z = CSprite2d::NearScreenZ + 0.0001f;
    vertex.rhw = CSprite2d::RecipNearClip;
    int r, g, b;

    // Placeholder colors for now. When I manage to find the original RGB values for all these I'll make them configurable as well.
    switch (color) {
    case 0: // RED
        r = plugin::color::Red.r; g = plugin::color::Red.g; b = plugin::color::Red.b; break;
    case 1: // GREEN
        r = plugin::color::Chartreuse.r; g = plugin::color::Chartreuse.g; b = plugin::color::Chartreuse.b; break;
    case 2: // BLUE
        r = plugin::color::DarkBlue.r; g = plugin::color::DarkBlue.g; b = plugin::color::DarkBlue.b; break;
    case 3: // WHITE
        r = plugin::color::White.r; g = plugin::color::White.g; b = plugin::color::White.b; break;
    case 4: // YELLOW
        r = plugin::color::Gold.r; g = plugin::color::Gold.g; b = plugin::color::Gold.b; break;
    case 5: // PURPLE
        r = plugin::color::Purple.r; g = plugin::color::Purple.g; b = plugin::color::Purple.b; break;
    case 6: // CYAN
        r = plugin::color::Cyan.r; g = plugin::color::Cyan.g; b = plugin::color::Cyan.b; break;
    case 7: // Depends on whether blip is friendly
        if (friendly) {
            r = plugin::color::DarkBlue.r; g = plugin::color::DarkBlue.g; b = plugin::color::DarkBlue.b;
        }
        else {
            r = plugin::color::Red.r; g = plugin::color::Red.g; b = plugin::color::Red.b;
        }
        break;
    case 8: // DESTINATION
        r = plugin::color::Gold.r; g = plugin::color::Gold.g; b = plugin::color::Gold.b; break;
    default:
        r = GPS_LINE_R; g = GPS_LINE_G; b = GPS_LINE_B; break;
    }

    vertex.emissiveColor = RWRGBALONG(r, g, b, GPS_LINE_A);
}

void GPSLine::renderPath(short color, bool friendly, short& nodesCount, bool& gpsShown, CNodeAddress* resultNodes, CVector2D* nodePoints, float& gpsDistance, RwIm2DVertex* lineVerts) {
    if (nodesCount <= 0) {
        return;
    }


    if (!FrontEndMenuManager.m_bDrawRadarOrMap
        && reinterpret_cast<D3DCAPS9 const*>(RwD3D9GetCaps())->RasterCaps & D3DPRASTERCAPS_SCISSORTEST)
    {
        RECT rect;
        CVector2D posn;
        CRadar::TransformRadarPointToScreenSpace(posn, CVector2D(-1.0f, -1.0f));
        rect.left = static_cast<LONG>(posn.x + 2.0f);
        rect.bottom = static_cast<LONG>(posn.y - 2.0f);
        CRadar::TransformRadarPointToScreenSpace(posn, CVector2D(1.0f, 1.0f));
        rect.right = static_cast<LONG>(posn.x - 2.0f);
        rect.top = static_cast<LONG>(posn.y + 2.0f);
        GetD3DDevice()->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        GetD3DDevice()->SetScissorRect(&rect);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);

    unsigned int vertIndex = 0;
    for (short i = 0; i < (nodesCount - 1); i++) {
        CVector2D point[4], shift[2];
        CVector2D dir = nodePoints[i + 1] - nodePoints[i];
        float angle = atan2(dir.y, dir.x);
        if (!FrontEndMenuManager.m_bDrawRadarOrMap) {
            shift[0].x = cosf(angle - 1.5707963f) * GPS_LINE_WIDTH;
            shift[0].y = sinf(angle - 1.5707963f) * GPS_LINE_WIDTH;
            shift[1].x = cosf(angle + 1.5707963f) * GPS_LINE_WIDTH;
            shift[1].y = sinf(angle + 1.5707963f) * GPS_LINE_WIDTH;
        }
        else {
            float mp = FrontEndMenuManager.m_fMapZoom - 140.0f;
            if (mp < 140.0f)
                mp = 140.0f;
            else if (mp > 960.0f)
                mp = 960.0f;
            mp = mp / 960.0f + 0.4f;
            shift[0].x = cosf(angle - 1.5707963f) * GPS_LINE_WIDTH * mp;
            shift[0].y = sinf(angle - 1.5707963f) * GPS_LINE_WIDTH * mp;
            shift[1].x = cosf(angle + 1.5707963f) * GPS_LINE_WIDTH * mp;
            shift[1].y = sinf(angle + 1.5707963f) * GPS_LINE_WIDTH * mp;
        }
        this->Setup2dVertex(lineVerts[vertIndex + 0], nodePoints[i].x + shift[0].x, nodePoints[i].y + shift[0].y, color, friendly);
        this->Setup2dVertex(lineVerts[vertIndex + 1], nodePoints[i + 1].x + shift[0].x, nodePoints[i + 1].y + shift[0].y, color, friendly);
        this->Setup2dVertex(lineVerts[vertIndex + 2], nodePoints[i].x + shift[1].x, nodePoints[i].y + shift[1].y, color, friendly);
        this->Setup2dVertex(lineVerts[vertIndex + 3], nodePoints[i + 1].x + shift[1].x, nodePoints[i + 1].y + shift[1].y, color, friendly);
        vertIndex += 4;
    }

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, lineVerts, 4 * (nodesCount - 1));

    if (!FrontEndMenuManager.m_bDrawRadarOrMap
        && reinterpret_cast<D3DCAPS9 const*>(RwD3D9GetCaps())->RasterCaps & D3DPRASTERCAPS_SCISSORTEST)
    {
        GetD3DDevice()->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }

    gpsDistance += DistanceBetweenPoints(FindPlayerCoors(0), ThePaths.GetPathNode(resultNodes[0])->GetNodeCoors());
    gpsShown = true;
}

// Check whether on BMX, will always return false if bmx support is enabled.
bool GPSLine::CheckBMX() {
    if (ENABLE_BMX)
        return false;
    
    return FindPlayerPed(0)->m_pVehicle->m_nVehicleSubClass == VEHICLE_BMX;
}

GPSLine::GPSLine() {
    // Logging stuff
    this->logfile.open("SA.GPS.LOG.txt", std::ios::out);

    // Load config values from file.
    iniFile.open("SA.GPS.CONF.ini", std::ios::in);
    iniParser.parse(iniFile);
    this->Log("INI config loaded:");
    iniParser.generate(this->logfile);
    iniParser.strip_trailing_comments();
    iniParser.interpolate();
    this->Log("INI config processed:");
    iniParser.generate(this->logfile);

    inipp::get_value(iniParser.sections["Navigation Config"], "Navigation line width", GPS_LINE_WIDTH);
    inipp::get_value(iniParser.sections["Navigation Config"], "Navigation line opacity", GPS_LINE_A);
    inipp::get_value(iniParser.sections["Navigation Config"], "Enable navigation on bicycles", ENABLE_BMX);
    inipp::get_value(iniParser.sections["Navigation Config"], "Navigation line removal proximity", DISABLE_PROXIMITY);

    inipp::get_value(iniParser.sections["Waypoint Config"], "Waypoint line red", GPS_LINE_R);
    inipp::get_value(iniParser.sections["Waypoint Config"], "Waypoint line green", GPS_LINE_G);
    inipp::get_value(iniParser.sections["Waypoint Config"], "Waypoint line blue", GPS_LINE_B);
    iniFile.close();

    for (int i = 0; i < 1024; i++) {
        pathNodesToStream[i] = 1;
    }

    for (int i = 0; i < 50000; i++) {
        pathNodes[i] = -1;
    }

    plugin::patch::SetPointer(0x44DE3C, &pathNodesToStream);
    plugin::patch::SetPointer(0x450D03, &pathNodesToStream);
    plugin::patch::SetPointer(0x451782, &pathNodes);
    plugin::patch::SetPointer(0x451904, &pathNodes);
    plugin::patch::SetPointer(0x451AC3, &pathNodes);
    plugin::patch::SetPointer(0x451B33, &pathNodes);
    plugin::patch::SetUInt(0x4518F8, 50000);
    plugin::patch::SetUInt(0x4519B0, 49950);

    if (GetModuleHandleA("SAMP.dll")) return; //don't run if SAMP

    /*
      Clears target blip when player reaches it.
    */

    plugin::Events::gameProcessEvent += [this]() {
        if (FrontEndMenuManager.m_nTargetBlipIndex
            && CRadar::ms_RadarTrace[LOWORD(FrontEndMenuManager.m_nTargetBlipIndex)].m_nCounter == HIWORD(FrontEndMenuManager.m_nTargetBlipIndex)
            && CRadar::ms_RadarTrace[LOWORD(FrontEndMenuManager.m_nTargetBlipIndex)].m_nBlipDisplay
            && FindPlayerPed(0)
            && DistanceBetweenPoints(CVector2D(FindPlayerCoors(0)),
                CVector2D(CRadar::ms_RadarTrace[LOWORD(FrontEndMenuManager.m_nTargetBlipIndex)].m_vecPos)) < DISABLE_PROXIMITY)
        {
            this->once = false;
            CRadar::ClearBlip(FrontEndMenuManager.m_nTargetBlipIndex);
            FrontEndMenuManager.m_nTargetBlipIndex = 0;
        }
    };

    

    plugin::Events::drawRadarOverlayEvent += [this]() {
        CPed* playa = FindPlayerPed(0);
        if (playa
            && playa->m_pVehicle
            && playa->m_nPedFlags.bInVehicle
            && playa->m_pVehicle->m_nVehicleSubClass != VEHICLE_PLANE
            && playa->m_pVehicle->m_nVehicleSubClass != VEHICLE_HELI
            && !CheckBMX()
            && FrontEndMenuManager.m_nTargetBlipIndex
            && CRadar::ms_RadarTrace[LOWORD(FrontEndMenuManager.m_nTargetBlipIndex)].m_nCounter == HIWORD(FrontEndMenuManager.m_nTargetBlipIndex)
            && CRadar::ms_RadarTrace[LOWORD(FrontEndMenuManager.m_nTargetBlipIndex)].m_nBlipDisplay)
        {
            CVector destPosn = CRadar::ms_RadarTrace[LOWORD(FrontEndMenuManager.m_nTargetBlipIndex)].m_vecPos;
            if (!this->once) {
                this->Log("TARGET POS: " + std::to_string(destPosn.x) + ", " + std::to_string(destPosn.y) + ", " + std::to_string(destPosn.z));
                this->once = true;
            }
            this->targetRouteShown = false;
            this->calculatePath(destPosn, targetNodesCount, t_ResultNodes, t_NodePoints, targetDistance);
            this->renderPath(-1, true, targetNodesCount, targetRouteShown, t_ResultNodes, t_NodePoints, targetDistance, t_LineVerts);
        }

        if (playa
            && playa->m_pVehicle
            && playa->m_nPedFlags.bInVehicle
            && playa->m_pVehicle->m_nVehicleSubClass != VEHICLE_PLANE
            && playa->m_pVehicle->m_nVehicleSubClass != VEHICLE_HELI
            && !CheckBMX()
            && CTheScripts::IsPlayerOnAMission())
        {
            this->Log("Looking for mission objective blip.");
            for (int i = 0; i < 174; i++) {
                tRadarTrace trace = CRadar::ms_RadarTrace[i];
                this->Log((int)trace.m_nBlipType + ", " + (int)trace.m_nRadarSprite);
                if (trace.m_nRadarSprite == 0 && trace.m_nBlipDisplay > 1 && DistanceBetweenPoints(playa->GetPosition(), trace.m_vecPos) > DISABLE_PROXIMITY) {
                    this->Log("Found mission objective blip.");
                    CVector destVec;
                    switch (trace.m_nBlipType) {
                    case 1:
                        destVec = CPools::GetVehicle(trace.m_nEntityHandle)->GetPosition();
                        break;
                    case 2:
                        destVec = CPools::GetPed(trace.m_nEntityHandle)->GetPosition();
                        break;
                    case 3:
                        destVec = CPools::GetObject(trace.m_nEntityHandle)->GetPosition();
                        break;
                    case 6: // Searchlights
                    case 8: // Airstripts
                    case 0: // NONE???
                        return;
                    case 7: // Pickups
                        this->Log("Pickup detected. Not providing GPS navigation.");
                        return;
                    default:
                        destVec = trace.m_vecPos;
                        break;
                    }
                    
                    this->missionRouteShown = false;
                    this->calculatePath(destVec, missionNodesCount, m_ResultNodes, m_NodePoints, missionDistance);
                    this->renderPath(trace.m_nColour, trace.m_bFriendly, missionNodesCount, missionRouteShown, m_ResultNodes, m_NodePoints, missionDistance, m_LineVerts);
                }
            }

            
        }

    };

    plugin::Events::shutdownRwEvent += [this]() {
        this->logfile.close();
    };
}

void GPSLine::Log(std::string val) {
    if (this->logLines < 96) {
        time_t timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char stime[128];
        strftime(stime, 128, "%c", localtime(&timenow));
        this->logfile << stime << " | " << val.c_str() << '\n';
    }
    else {
        this->logfile.close();
        this->logfile.open("SA.GPS.CONF.ini", std::ios::out);
        this->logLines = 0;
        Log(val);
    }
}