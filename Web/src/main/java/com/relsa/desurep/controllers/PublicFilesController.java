package com.relsa.desurep.controllers;

import com.relsa.desurep.services.FileStorageService;
import com.relsa.desurep.models.File;
import com.relsa.desurep.services.FileSerchService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.servlet.mvc.support.RedirectAttributes;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

@Controller
public class PublicFilesController {
    @Autowired
    private FileStorageService fileStorageService;
    @Autowired
    private FileSerchService fileSerchService;

    @GetMapping("/public")
    public String getPublicPage(@RequestParam(value = "publicationName", required=false) String publicationName,
                                  @RequestParam(value = "authorName", required=false) String authorName,
                                  @RequestParam(value = "uploadDate", required=false) String uploadDate,
                                  @RequestParam(value = "theme", required=false) String theme,
                                  @RequestParam(value = "publicationDate", required=false) String publicationDate,
                                  Model model) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        List<File> files = fileStorageService.getPublicFiles();
        File exFile = fileSerchService.createExemplaryFile(publicationName, authorName, theme,
                publicationDate, uploadDate);
        if (exFile != null)
            files = fileSerchService.initSerch(files, exFile);
        if (files == null)
            files = new ArrayList<File>();
        model.addAttribute("files", files);
        model.addAttribute("exFile", Objects.requireNonNullElseGet(exFile, () -> new File("Any_value",
                "Any_value", "Any_value", "Any_value",
                "Any_value")));
        return "public_files";
    }

    @PostMapping("/public")
    public String uploadFile(@RequestParam(value = "publicationName", required=false) String publicationName,
                             @RequestParam(value = "authorName", required=false) String authorName,
                             @RequestParam(value = "uploadDate", required=false) String uploadDate,
                             @RequestParam(value = "theme", required=false) String theme,
                             @RequestParam(value = "publicationDate", required=false) String publicationDate,
                             @RequestParam(value = "res", required=false) String res,
                             RedirectAttributes redirectAttrs) {
        if(Objects.equals(res, "sub")) {
            redirectAttrs.addAttribute("publicationName", publicationName);
            redirectAttrs.addAttribute("authorName", authorName);
            redirectAttrs.addAttribute("uploadDate", uploadDate);
            redirectAttrs.addAttribute("theme", theme);
            redirectAttrs.addAttribute("publicationDate", publicationDate);
        }
        return "redirect:/public";
    }
}