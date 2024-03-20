package com.relsa.desurep.controllers.FileControllers;

import com.relsa.desurep.models.File;
import com.relsa.desurep.services.FileStorageService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.servlet.mvc.support.RedirectAttributes;

import java.util.Objects;

@Controller
public class FileEditController {
    @Autowired
    FileStorageService fileStorageService;
    @GetMapping("/editfile")
    public String editFile(Model model,
                           @RequestParam String filename) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        File file = fileStorageService.getFile(filename, authentication.getName(), 1);
        if (file != null) {
            model.addAttribute(file);
            model.addAttribute("description", file.getDescription());
            return "file_pages/file_edit_page";
        }
        return "file_pages/file_error_page";
    }

    @PostMapping("/editfile")
    public String updateFile(@RequestParam(value = "filename", required=false) String filename,
                             @RequestParam(value = "publicationName", required=false) String publicationName,
                             @RequestParam(value = "author", required=false) String author,
                             @RequestParam(value = "theme", required=false) String theme,
                             @RequestParam(value = "yearOfPublication", required=false) String yearOfPublication,
                             @RequestParam(value = "description", required=false) String description,
                             @RequestParam(value = "isPublic", required=false) Boolean isPublic,
                             @RequestParam(value = "res", required=false) String res,
                             RedirectAttributes redirectAttrs) {
        if(Objects.equals(res, "reset"))
            return "redirect:/editfile/?filename=" + filename;
        if(Objects.equals(res, "sub")) {
            boolean publicFlag = isPublic != null;
            Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
            fileStorageService.updateFile(filename, author, publicationName, theme, yearOfPublication, publicFlag, description);
        }
        return "redirect:/";
    }
}
