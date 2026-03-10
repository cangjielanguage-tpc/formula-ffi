/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 */
import { requireCJLib } from "libark_interop_loader.so";

export declare interface CustomLib {
  latexStringToImage(latexMathTextString: string, latexMathTextSize: number, latexMathTextColor: number, latexMathBackGroupColor: number, latexMathColorFormat: number, resPath: string): Promise<ArrayBuffer>
  latexStringToImageWithError(latexMathTextString: string, latexMathTextSize: number, latexMathTextColor: number, latexMathBackGroupColor: number, latexMathColorFormat: number, resPath: string): Promise<TeXParseResult>
}

let lib = requireCJLib("libformula.so") as CustomLib

/**
 * 图片格式枚举
 */
export enum LatexMathColorFormat {
  COLOR_FORMAT_RGB_565, // RGB_565
  COLOR_FORMAT_BGRA_8888 // BGRA_8888
}

/**
 * TeX解析错误码枚举
 */
export enum TeXErrorCode {
  Success = 0,
  SyntaxError = 1,
  InvalidMatrixError = 2,
  InvalidDelimiterError = 3,
  TeXError = 4,
  UnknownError = 99
}

/**
 * TeX解析结果
 */
export interface TeXParseResult {
  isSuccess: boolean
  imageBytes: ArrayBuffer
  formula: string
  errorCode: TeXErrorCode
  errorMessage: string
}

/**
 * 数学公式文本转换图片数组数据
 *
 * @param latexMathTextString 数学公式文本
 * @param latexMathTextSize 文本大小
 * @param latexMathTextColor 文本颜色
 * @param latexMathBackGroupColor 背景色
 * @param latexMathColorFormat 图片格式
 * @param resPath 字体资源路径
 * @returns 图片数组数据
 */
export function latexStringToImage(latexMathTextString: string, latexMathTextSize: number, latexMathTextColor: number, latexMathBackGroupColor: number, latexMathColorFormat: LatexMathColorFormat, resPath: string = "/data/storage/el1/bundle/entry/resources/resfile/res"): Promise<ArrayBuffer> {
  let colorFormat = 5
  if (latexMathColorFormat == LatexMathColorFormat.COLOR_FORMAT_RGB_565) {
    colorFormat = 2
  } else {
    colorFormat = 5
  }
  return lib.latexStringToImage(latexMathTextString, latexMathTextSize, latexMathTextColor, latexMathBackGroupColor, colorFormat, resPath)
}

/**
 * 数学公式文本转换图片数组数据（带错误信息）
 * 当公式解析失败时，返回详细的错误信息
 *
 * @param latexMathTextString 数学公式文本
 * @param latexMathTextSize 文本大小
 * @param latexMathTextColor 文本颜色
 * @param latexMathBackGroupColor 背景色
 * @param latexMathColorFormat 图片格式
 * @param resPath 字体资源路径
 * @returns TeXParseResult 解析结果，包含成功/失败状态、图片数据或错误信息
 */
export function latexStringToImageWithError(latexMathTextString: string, latexMathTextSize: number, latexMathTextColor: number, latexMathBackGroupColor: number, latexMathColorFormat: LatexMathColorFormat, resPath: string = "/data/storage/el1/bundle/entry/resources/resfile/res"): Promise<TeXParseResult> {
  let colorFormat = 5
  if (latexMathColorFormat == LatexMathColorFormat.COLOR_FORMAT_RGB_565) {
    colorFormat = 2
  } else {
    colorFormat = 5
  }
  return lib.latexStringToImageWithError(latexMathTextString, latexMathTextSize, latexMathTextColor, latexMathBackGroupColor, colorFormat, resPath)
}