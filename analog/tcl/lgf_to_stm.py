#!/usr/intel/pkgs/python/3.5.1/bin/python -u

import argparse
import subprocess
import shlex
import os
import sys
import glob
import gdsii
import layermap

if not os.getenv('LAYGENWARD'):
   errmsg ('LAYGENWARD is not ropts(')
if not os.getenv('COLLATERAL'):
   errmsg ('COLLATERAL is not ropts(')

sys.path.append(os.environ['LAYGENWARD']+'/LgfApi')

import LgfApiPy3 as LgfApi

NET_NAME_PROPERTY = 126


def mapLayer(laygenLayer):
    map = {'wirepoly':'poly', 'nwirediff':'ndiff', 'pwirediff':'pdiff', 'vcg':'viag', 'vct':'viat'}

    if laygenLayer in map.keys():
        return map[laygenLayer]
    return laygenLayer


def __write_rect(writter, layerString, rect):
    xmin, ymin, xmax, ymax = [int(v) for v in rect]
    polygon = [xmin, ymin, xmax, ymin, xmax, ymax, xmin, ymax, xmin, ymin]

    writter.polygon(layerString, polygon)


def _write_rect(writter, layerLaygen, text, rect, lm):
    xmin, ymin, xmax, ymax = [int(v) for v in rect]
    layer = mapLayer(layerLaygen)
    
    layerName = (layer, 'drawing')

    if layerName not in lm:
        print('Layer ',layer,' purpose drawing not found in map file')
        return

    layerNumber = lm[layerName]

    layerIndex = layerNumber[0];
    dataType   = layerNumber[1];

#    layerString = str(layerIndex) + ":" + str(dataType)
    layerString = ":".join([str(x) for x in [layerIndex, dataType]])

    __write_rect(writter, layerString, rect)

    if (xmax-xmin) < (ymax-ymin):
       rma = gdsii.GdsiiWriter.RMA(False, 0.002, 90.0)
    else:
       rma = gdsii.GdsiiWriter.RMA(False, 0.002, 0.0)

    writter.text(layerString, [(xmax + xmin) // 2, (ymax + ymin) // 2], text, rma)
    writter.property(NET_NAME_PROPERTY, text)

def mapNet(net, netMap):
    if net in netMap.keys():
        return netMap[net]
    return net

def transformRect(rect, transform):
    w = rect[2]-rect[0]
    h = rect[3]-rect[1]
    oldxl = rect[0]
    oldyl = rect[1]
    oldxh = rect[2]
    oldyh = rect[3];

    newxl = oldxl + transform[0];
    newyl = oldyl + transform[1];
    newxh = oldxh + transform[0];
    newyh = oldyh + transform[1];

    if transform[2] == "ROT_0_REF_0":
        pass
    elif transform[2] == "ROT_0_REF_Y":
        newxh = x - oldxl
        newxl = newxh - w
    elif transform[2] == "ROT_0_REF_X":
        newyh = y - oldyl
        newyl = newyh - h
    elif transform[2] == "ROT_180_REF_0":
        newxh = x - oldxl;
        newxl = newxh - w;
        newyh = y - oldyl;
        newyl = newyh - h;
    else:
        raise Exception("Bad Transformation "+transform[2])

    return [newxl, newyl, newxh, newyh]


def write_wire(writter, wire, lm, netMap={}, transform=(0,0,'ROT_0_REF_0')):
    net = mapNet(wire.getValue('net'), netMap)
    layer = wire.getValue('layer')
    rect =  transformRect(LgfApi.LgfApi.str_2_intvec(wire.getValue('rect')), transform)
    _write_rect(writter, layer, net, rect, lm)

def write_device(writter, device, arch, lm, netMap={}, transform=(0,0,'ROT_0_REF_0')):
    name = device.getValue("name")
    nets = list(LgfApi.LgfApi.str_2_strvec(device.getValue("nets")))
    snet = nets[0]
    gnet = nets[1]
    dnet = nets[2]
    devtype = device.getValue("type")
    model = device.getValue("model")
    w_i = int(device.getValue("w"))
    w = float(w_i)/10000
    l_i = int(device.getValue("l"))
    l = float(l_i)/10000
    flip = device.getValue("f") == "i"
    
    r = int(device.getValue("r"))
    x = int(device.getValue("x"))
    y = int(device.getValue("y"))
    devPolygons = LgfApi.LgLdmUtils.getDevicePolygons(arch,model,r,x,y,w_i)
    
#         print("devPolyons:",devPolygons)
    
    diffstr = "ndiff"
    bNetName = "vss"
    if devtype == 'p':
        diffstr = "pdiff"
        bNetName = "vcc"
        
    rectList = []
        
    for i in range(0, len(devPolygons), 4):
        rectList.append(transformRect(devPolygons[i:i+4], transform))

    for i in range(3):
        nets[i] = mapNet(nets[i], netMap)
            
    for i in range(0, len(rectList)-3):
        _write_rect(writter, diffstr, bNetName, rectList[i], lm)
                
    _write_rect(writter, "diffcon",  nets[0], rectList[len(rectList)-3], lm)
    _write_rect(writter, "wirepoly", nets[1], rectList[len(rectList)-2], lm)
    _write_rect(writter, "diffcon",  nets[2], rectList[len(rectList)-1], lm)


def write_child_cell(writter, cell, arch, lm, netMap, transform):
    for wire in cell.getChildConstructs('Wire'):
        write_wire(writter, wire, lm, netMap, transform)

    for device in cell.getChildConstructs('Device'):
        write_device(writter, device, arch, lm, netMap, transform)


def write_cell(writter, cell, arch, lm):
    cellBbox = LgfApi.LgfApi.str_2_intvec(cell.getConstructs('Cell')[0].getValue('bbox'))
    __write_rect(writter, "50:0", cellBbox)

    for instance in cell.getConstructs('Instance'):
        netMap = {}

        for ipin in instance.getChildConstructs('IPin'):
            netMap[ipin.getValue('name')] = ipin.getValue('extnet')        

        master = instance.getValue('master')

        x = int(instance.getValue('x'))
        y = int(instance.getValue('y'))
        rot = '0'
        ref = '0'
        if instance.hasValue('rot'):
            rot = instance.getValue('rot')
        if instance.hasValue('ref'):
            ref = instance.getValue('ref')
        
        orientStr = "ROT_" + rot + "_REF_" + ref

        transform = (x,y,orientStr)

        for subcell in cell.getConstructs('InstMaster'):
            subcellmaster = subcell.getValue('name')
            if subcellmaster != master: continue
            write_child_cell(writter, subcell, arch, lm, netMap, transform)
            break
    
    for wire in cell.getConstructs('Wire'):
        # Nikolai; Feb 7, 2019; skip invisible wires
        if (wire.hasValue('invisible') and wire.getValue('invisible') != 0):
           #print('Skipped invisible wire ' + wire.getValue('layer'))
           continue
        write_wire(writter, wire, lm)

    for device in cell.getConstructs('Device'):
        write_device(writter, device, arch, lm)

def lgf_to_stm(lgffile, stmfile, arch):
    print('I', 'Create block - ' + lgffile)
    lgp = LgfApi.LgParser('LGF')
    lgp.readFile(lgffile)

    cell = lgp.getConstructs('Cell')[0]
    cname = cell.getValue('name')

    #look for a layer map file
    mapfiles = [mapfile for mapfile in os.listdir(os.environ['COLLATERAL']+'/arch/'+arch) if mapfile.endswith('.map')]

    if len(mapfiles) == 0:
        print('layer map (*.map) file is missing from COLLATERAL area')
        raise

    lm = layermap.read_layer_map_from_file(os.environ['COLLATERAL']+'/arch/'+arch+'/'+mapfiles[0])

    writter = gdsii.GdsiiWriter(stmfile)
    writter.units(10000) #dbunits Angstrom
    writter.begin_cell(cname)

    write_cell(writter, lgp, arch, lm)
        
    writter.end_cell()    
    writter.end_stream()


    
#     oanets = {}
#     oanets['vcc']    = oa.oaNet.create(block, 'vcc', oa.oacGroundSigType, 1)
#     oanets['vss']    = oa.oaNet.create(block, 'vss', oa.oacPowerSigType, 1)
#     oanets['!float'] = oa.oaNet.create(block, '!float', oa.oacSignalSigType, 1)
#     oanets['!kor']   = oa.oaNet.create(block, '!kor', oa.oacSignalSigType, 1)
#     for nets in lgp.getConstructs('Nets'):
#         for net in LgfApi.LgfApi.str_2_strvec(nets.getValue('nets')):
# 	    oanets[net] = oa.oaNet.create(block, net)

#     for wire in lgp.getConstructs('Wire'):
#         net = wire.getValue('net')
#         layer = wire.getValue('layer')
#         rect =  LgfApi.LgfApi.str_2_strvec(wire.getValue('rect'))
# 	if (not laygen2oa.has_key(layer)): 
# 	    print 'Unknown oa layer map : ' + layer
# 	    continue

#         (lname, ldir, lid, lnum) = laygen2oa[layer]
#         (wxl, wyl, wxh, wyh) = [int(rect[0]), int(rect[1]), int(rect[2]), int(rect[3])]

#         oabox = oa.oaBox(wxl, wyl, wxh, wyh)
#         oarect = oa.oaRect.create(block, lnum, oa.oavPurposeNumberDrawing, oabox)

#         torig = oa.oaPoint((wxl + wxh) / 2, (wyl + wyh) / 2)
#         if (ldir == 'H'):
# 	    ori = oa.oacR0
# 	    height = get_text_height(net, oa.oaBox.getHeight(oabox))
# 	else:
# 	    ori = oa.oacR90
# 	    height = get_text_height(net, oa.oaBox.getWidth(oabox))
# 	oatext = oa.oaText.create(block, lnum, oa.oavPurposeNumberDrawing, net, torig, oa.oacCenterCenterTextAlign, ori, oa.oacEuroStyleFont, height)
	
#         if lid:
#             oaxCtk.ShapeMPT(oarect).setColor(lid)
#             oaxCtk.ShapeMPT(oatext).setColor(lid)
#         oa.oaConnFig.addToNet(oarect, oanets[net])

#     oa.oaDesign.save(design)
#     oa.oaDesign.close(design)
#     del lgp
#    return cname


def main():
    print("")
    ll = '#  LGF to STM Convertor #'
    print('#' * len(ll))
    print(ll)
    print('#' * len(ll))

    parser = argparse.ArgumentParser( description="Converts an lgf file to a stm file.")
    parser.add_argument('-l', '--lgf', required=True, type=str, help='Path to the .lgf file to read.')
    parser.add_argument('architecture', type=str, help='Architecture: g0m/g0s/etc')

    args = parser.parse_args()

    stmfile = args.lgf.replace('.lgf','.stm')
    
    lgf_to_stm(args.lgf, stmfile, args.architecture)

#try:
#    main()
#except Exception as e:
#    print('E', str(e))

main()
